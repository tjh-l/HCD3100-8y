/**
* @file
* @brief                hudi sound interface
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <hudi_com.h>
#include <hudi_log.h>

#include "hudi_audio_inter.h"

static pthread_mutex_t g_hudi_snd_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_snd_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_snd_mutex);

    return 0;
}

static int _hudi_snd_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_snd_mutex);

    return 0;
}

static int _hudi_snd_dev_start(int fd, struct snd_pcm_params *config, unsigned int pcm_source)
{
    snd_pcm_uframes_t poll_size = 0;

    if (!config->channels || !config->bitdepth)
    {
        hudi_log(HUDI_LL_ERROR, "Incorrect parameters\n");
        return 0;
    }

    poll_size = 24000 / (config->channels * config->bitdepth / 8);
    config->pcm_source = pcm_source;
    
    if (ioctl(fd, SND_IOCTL_HW_PARAMS, config) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "SND Parameters set fail\n");
        return -1;
    }

    ioctl(fd, SND_IOCTL_AVAIL_MIN, &poll_size);
    ioctl(fd, SND_IOCTL_START, 0);

    return 0;
}

static int _hudi_snd_dev_stop(int fd)
{
    if (fd > 0)
    {
        ioctl(fd, SND_IOCTL_DROP, 0);
        ioctl(fd, SND_IOCTL_HW_FREE, 0);
    }

    return 0;
}

static int _hudi_snd_dev_feed(hudi_snd_instance_t *inst, int fd, struct snd_xfer *xfer)
{
    int ret = -1;
    struct pollfd pfd = {0};

    if (!inst->i2so_feeded)
    {
        pfd.fd = fd;
        pfd.events = POLLOUT | POLLWRNORM;
        while (ret < 0)
        {
            ret = ioctl(fd, SND_IOCTL_XFER, xfer);
            if (ret < 0)
            {
                usleep(2000);
            }
        }

        inst->i2so_feeded = 1;
    }
    else
    {
        ret = ioctl(fd, SND_IOCTL_XFER, xfer);
        if (ret < 0)
        {
            hudi_log(HUDI_LL_ERROR, "SND xfer fail\n");
            ioctl(fd, SND_IOCTL_DROP, 0);
            ioctl(fd, SND_IOCTL_START, 0);
        }
    }

    return ret;
}

int hudi_snd_open(hudi_handle *handle, unsigned int audsink)
{
    hudi_snd_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid adec parameters\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    inst = (hudi_snd_instance_t *)malloc(sizeof(hudi_snd_instance_t));
    memset(inst, 0, sizeof(hudi_snd_instance_t));
    inst->fd_i2so = -1;
    inst->fd_spo = -1;
    inst->fd_kshm = -1;

    if (audsink & AUDSINK_SND_DEVBIT_I2SO)
    {
        inst->fd_i2so = open(HUDI_SND_I2SO_DEV, O_RDWR);
        if (inst->fd_i2so < 0)
        {
            free(inst);
            hudi_log(HUDI_LL_ERROR, "Open %s fail\n", HUDI_SND_I2SO_DEV);
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (audsink & AUDSINK_SND_DEVBIT_SPO)
    {
        inst->fd_spo = open(HUDI_SND_SPO_DEV, O_RDWR);
        if (inst->fd_spo < 0)
        {
            if (inst->fd_i2so >= 0)
            {
                close(inst->fd_i2so);
            }
            free(inst);
            hudi_log(HUDI_LL_ERROR, "Open %s fail\n", HUDI_SND_SPO_DEV);
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_close(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (inst->started)
        {
            _hudi_snd_dev_stop(inst->fd_i2so);
        }
        close(inst->fd_i2so);
    }

    if (inst->fd_spo >= 0)
    {
        if (inst->started)
        {
            _hudi_snd_dev_stop(inst->fd_spo);
        }
        close(inst->fd_spo);
    }

    memset(inst, 0, sizeof(hudi_snd_instance_t));
    free(inst);

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_start(hudi_handle handle, struct snd_pcm_params *config)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    if (inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND already started\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (0 != _hudi_snd_dev_start(inst->fd_i2so, config, SND_PCM_SOURCE_AUDPAD))
        {
            hudi_log(HUDI_LL_ERROR, "SND I2SO start fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (inst->fd_spo >= 0)
    {
        if (inst->fd_i2so >= 0)
        {
            if (0 != _hudi_snd_dev_start(inst->fd_spo, config, SND_SPO_SOURCE_I2SODMA))
            {
                hudi_log(HUDI_LL_ERROR, "SND SPO start fail\n");
                _hudi_snd_mutex_unlock();
                return -1;
            }
        }
        else
        {
            if (0 != _hudi_snd_dev_start(inst->fd_spo, config, SND_SPO_SOURCE_SPODMA))
            {
                hudi_log(HUDI_LL_ERROR, "SND SPO start fail\n");
                _hudi_snd_mutex_unlock();
                return -1;
            }
        }
    }

    inst->started = 1;

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_stop(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not start\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        _hudi_snd_dev_stop(inst->fd_i2so);
    }

    if (inst->fd_spo >= 0)
    {
        _hudi_snd_dev_stop(inst->fd_spo);
    }

    inst->started = 0;

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_pause(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not start\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (0 != ioctl(inst->fd_i2so, SND_IOCTL_PAUSE, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND I2SO pause fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (inst->fd_spo >= 0)
    {
        if (0 != ioctl(inst->fd_spo, SND_IOCTL_PAUSE, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND SPO pause fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_resume(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not start\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (0 != ioctl(inst->fd_i2so, SND_IOCTL_RESUME, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND I2SO resume fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (inst->fd_spo >= 0)
    {
        if (0 != ioctl(inst->fd_spo, SND_IOCTL_RESUME, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND SPO resume fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_feed(hudi_handle handle, struct snd_xfer *pkt)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not start\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = _hudi_snd_dev_feed(inst, inst->fd_i2so, pkt);
        if (ret < 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (inst->fd_spo >= 0 && inst->fd_i2so < 0)
    {
        ret = _hudi_snd_dev_feed(inst, inst->fd_spo, pkt);
        if (ret < 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_flush(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not start\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (0 != ioctl(inst->fd_i2so, SND_IOCTL_DROP, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND I2SO flush fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    if (inst->fd_spo >= 0)
    {
        if (0 != ioctl(inst->fd_spo, SND_IOCTL_DROP, 0))
        {
            hudi_log(HUDI_LL_ERROR, "SND SPO flush fail\n");
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }

    _hudi_snd_mutex_unlock();

    return 0;
}

#ifdef __HCRTOS__
int hudi_snd_record_start(hudi_handle handle, int rec_buf_size)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    if (inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND record already started\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (ioctl(inst->fd_i2so, SND_IOCTL_SET_RECORD, rec_buf_size) != 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }

        if (ioctl(inst->fd_i2so, KSHM_HDL_ACCESS, &inst->aud_hdl) != 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }    
    
    inst->started = 1;

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_record_read(hudi_handle handle, char *buf, int size)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = kshm_read((kshm_handle_t)&inst->aud_hdl, buf, size);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}
#else
int hudi_snd_record_start(hudi_handle handle, int rec_buf_size)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    if (inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND record already started\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        if (ioctl(inst->fd_i2so, SND_IOCTL_SET_RECORD, rec_buf_size) != 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }

        if (ioctl(inst->fd_i2so, KSHM_HDL_ACCESS, &inst->aud_hdl) != 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }

        if (inst->fd_kshm == -1)
        {
            inst->fd_kshm = open(HUDI_SND_KSHM , O_RDONLY);
            if (inst->fd_kshm < 0)
            {
                _hudi_snd_mutex_unlock();
                return -1;
            }
        }
        
        if (ioctl(inst->fd_kshm, KSHM_HDL_SET, &inst->aud_hdl) != 0)
        {
            _hudi_snd_mutex_unlock();
            return -1;
        }
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }
    
    inst->started = 1;

    _hudi_snd_mutex_unlock();

    return 0;
}

int hudi_snd_record_read(hudi_handle handle, char *buf, int size)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_kshm >= 0)
    {
        ret = read(inst->fd_kshm, buf, size);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}
#endif

int hudi_snd_record_stop(hudi_handle handle)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited || inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so > 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_FREE_RECORD, 0);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    if (inst->fd_kshm >= 0)
    {
        close(inst->fd_kshm);
        inst->fd_kshm = -1;
    }

    inst->started = 0;
    
    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_volume_get(hudi_handle handle, unsigned char *volume)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited || !volume)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_GET_VOLUME, volume);
    }
    else
    {
        ret = ioctl(inst->fd_spo, SND_IOCTL_GET_VOLUME, volume);
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_volume_set(hudi_handle handle, unsigned char volume)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_VOLUME, &volume);
    }
    else
    {
        ret = ioctl(inst->fd_spo, SND_IOCTL_SET_VOLUME, &volume);
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_mute_set(hudi_handle handle, unsigned int mute)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_MUTE, mute);
    }
    else
    {
        ret = ioctl(inst->fd_spo, SND_IOCTL_SET_MUTE, mute);
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_drc_param_get(hudi_handle handle, float *peak_dBFS, float *gain_dBFS)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    struct snd_drc_setting setting = { 0 };
    int ret = -1;

    if (!inst || !inst->inited || !peak_dBFS || !gain_dBFS)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_GET_DRC_PARAM, &setting);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_drc_param_set(hudi_handle handle, float peak_dBFS, float gain_dBFS)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    struct snd_drc_setting setting = { 0 };
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    setting.gain_dBFS = gain_dBFS;
    setting.peak_dBFS = peak_dBFS;

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_DRC_PARAM, &setting);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_dual_tone_set(hudi_handle handle, int on, int bass_index, int treble_index, snd_twotone_mode_e mode)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    struct snd_twotone tt = {0};
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    tt.tt_mode = mode;
    tt.onoff = on;
    tt.bass_index = bass_index;
    tt.treble_index = treble_index;

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_TWOTONE, &tt);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_balance_set(hudi_handle handle, int on, int balance_index)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    struct snd_lr_balance lr = {0};
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    lr.lr_balance_index = balance_index;
    lr.onoff = on;

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_LR_BALANCE, &lr);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_equalizer_set(hudi_handle handle, int on)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_EQ_ONOFF, on);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_equalizer_band_set(hudi_handle handle, int band, int cutoff, int q, int gain)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    struct snd_eq_band_setting band_setting= {0};
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    band_setting.band = band;
    band_setting.cutoff = cutoff;
    band_setting.q = q;
    band_setting.gain = gain;

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_EQ_BAND, &band_setting);
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "SND not support\n");
        _hudi_snd_mutex_unlock();
        return -1;
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_hw_info_get(hudi_handle handle, struct snd_hw_info *info)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited || !info)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_GET_HW_INFO, info);
    }
    else
    {
        ret = ioctl(inst->fd_spo, SND_IOCTL_GET_HW_INFO, info);
    }

    _hudi_snd_mutex_unlock();

    return ret;
}

int hudi_snd_auto_mute_set(hudi_handle handle, unsigned int mute)
{
    hudi_snd_instance_t *inst = (hudi_snd_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_snd_mutex_lock();

    if (inst->fd_i2so >= 0)
    {
        ret = ioctl(inst->fd_i2so, SND_IOCTL_SET_AUTO_MUTE_ONOFF, mute);
    }
    else
    {
        ret = ioctl(inst->fd_spo, SND_IOCTL_SET_AUTO_MUTE_ONOFF, mute);
    }

    _hudi_snd_mutex_unlock();

    return ret;
}
