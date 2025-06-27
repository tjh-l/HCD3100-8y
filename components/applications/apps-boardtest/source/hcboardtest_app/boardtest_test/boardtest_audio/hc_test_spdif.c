#include "hc_test_getfile.h"

static volatile bool user_exit;
static const char* detailstr;

static int hc_test_i2so_open(int rate, int channels, int bitdepth, int format)
{
    struct snd_pcm_params params = {0};
    int i2so_fd;

    i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (i2so_fd < 0) {
        printf("open i2so failed\n");
        return i2so_fd;
    }

    params.access          = SND_PCM_ACCESS_RW_INTERLEAVED;
    params.rate            = rate;
    params.channels        = channels;
    params.bitdepth        = bitdepth;
    params.format          = format;
    params.sync_mode       = AVSYNC_TYPE_FREERUN;
    params.align           = 0;
    params.period_size     = 1536;
    params.periods         = 8;
    params.start_threshold = 1;

    ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);
    ioctl(i2so_fd, SND_IOCTL_START, 0);

    return i2so_fd;
}

static int hc_test_spdif_open(int rate, int channels, int bitdepth, int format)
{
    struct snd_pcm_params params = {0};
    int spo_fd;

    spo_fd = open("/dev/sndC0spo", O_WRONLY);
    if (spo_fd < 0) {
        printf("open spo failed\n");
        return spo_fd;
    }

    params.access          = SND_PCM_ACCESS_RW_INTERLEAVED;
    params.rate            = rate;    
    params.channels        = channels;
    params.bitdepth        = bitdepth;
    params.format          = format;  
    params.sync_mode       = AVSYNC_TYPE_FREERUN;
    params.align           = SND_PCM_ALIGN_RIGHT;
    params.period_size     = 1536;
    params.periods         = 8;
    params.start_threshold = 1;
    params.pcm_source      = SND_SPO_SOURCE_I2SODMA;
    ioctl(spo_fd, SND_IOCTL_HW_PARAMS, &params);
    ioctl(spo_fd, SND_IOCTL_START, 0);

    return spo_fd;
}

static int hc_test_spdif_audiotest(void)
{
    user_exit = 0;
    char *path = NULL;
    FILE *wav_fd = NULL;
    struct wave_header header = {0};
    int rate = 0;
    int channels = 0;
    int bitdepth = 0;
    int format = 0;
    int read_size = 0;
    int i2so_fd = -1;
    int spo_fd = -1;
    void *buf = NULL;
    struct snd_xfer xfer = {0};
    int size = 0;
    int i = 0;
    int ret = BOARDTEST_PASS;
    detailstr = NULL;
    
    hc_test_findfile(&path,".wav");
    if(!path) {
        detailstr = "The file path for wav could not be found.";
        ret = BOARDTEST_ERROR_MOLLOC_MEMORY;
        goto close;
    }

    wav_fd = fopen(path, "rb");
    if (!wav_fd) {
        detailstr = "Failed to open the wav file.";
        ret =  BOARDTEST_ERROR_OPEN_FILE;
        goto close;
    }

    if (fread(&header, sizeof(struct wave_header), 1, wav_fd) == 0) {
        detailstr = "Parse wav failed.";
        ret =  BOARDTEST_ERROR_READ_FILE;
        goto close;
    }

    rate = header.wav_fmt.sample_rate;
    channels = header.wav_fmt.channels_num;
    bitdepth = header.wav_fmt.bits_per_sample;
    format = SND_PCM_FORMAT_S16_LE;
    read_size = 10248;
    i2so_fd = hc_test_i2so_open(rate, channels, bitdepth, format);
    if (i2so_fd < 0) {
        ret =  BOARDTEST_ERROR_OPEN_DEVICE;
        goto close;
    }

    spo_fd = hc_test_spdif_open(rate, channels, bitdepth, format);
    if (spo_fd < 0) {
        detailstr = "Open i2so failed.";
        ret = BOARDTEST_ERROR_OPEN_DEVICE;
        goto close;
    }

    buf = malloc(read_size);
    if(!buf) {
        detailstr = "Malloc memory failed.";
        ret =  BOARDTEST_ERROR_MOLLOC_MEMORY;
        goto close;
    }

    create_boardtest_passfail_mbox(BOARDTEST_SPDIF_AUDIO_TEST);
    while (!user_exit) {
        size = fread(buf, 1, read_size, wav_fd);
        if (size <= 0) {
            break;
        }
        xfer.data   = buf;
        xfer.frames = size / (channels * bitdepth / 8);
        while (ioctl(i2so_fd, SND_IOCTL_XFER, &xfer) < 0) {
            msleep(20);
        }
    }
    
    if(ioctl(i2so_fd, SND_IOCTL_DROP, 0) < 0) {
        detailstr = "Close i2so dev failed.";
        ret =  BOARDTEST_ERROR_IOCTL_DEVICE;
        goto close;
    }
    if(ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0) < 0) {
        detailstr = "Close i2so dev failed.";
        ret =  BOARDTEST_ERROR_IOCTL_DEVICE;
        goto close;
    }

close:
    if (path)
        free(path);
    if (wav_fd)
        fclose(wav_fd);
    if (i2so_fd >= 0)
        close(i2so_fd);
    if (spo_fd >= 0)
        close(spo_fd);
    if (buf)
        free(buf);

    return ret;
}

static int hc_test_spdif_testexit(void)
{
    user_exit = true;
    write_boardtest_detail(BOARDTEST_SPDIF_AUDIO_TEST, detailstr);
    return BOARDTEST_PASS;
}

static int hc_boardtest_spdif_auto_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "SPDIF_AUDIO_TEST";
    test->sort_name = BOARDTEST_SPDIF_AUDIO_TEST;
    test->init = NULL;
    test->run = hc_test_spdif_audiotest;
    test->exit = hc_test_spdif_testexit;
    test->tips = "Whether there is sound output?";

    hc_boardtest_module_register(test);

    return BOARDTEST_PASS;
}

__initcall(hc_boardtest_spdif_auto_register)
