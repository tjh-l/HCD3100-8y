/**
* @file
* @brief                hudi dsc interface
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
#include <malloc.h>

#include <hudi_com.h>
#include <hudi_log.h>

#include "hudi_dsc_inter.h"

#ifdef __HCRTOS__
static int _hudi_dsc_buf_alloc(hudi_dsc_instance_t *inst)
{
    inst->dsc_buf = (unsigned char*)memalign(32, inst->config.dsc_buf_len);
    if (!inst->dsc_buf)
    {    
        return -1;
    }
    
    return 0;
}

static void _hudi_dsc_buf_free(hudi_dsc_instance_t *inst)
{
    if(inst->dsc_buf)
    {
        free(inst->dsc_buf);
        inst->dsc_buf = NULL;
    }
}

static int _hudi_dsc_common_crypt(hudi_dsc_instance_t* inst, unsigned char *input, unsigned char *output, unsigned int len, int mode)
{
    struct dsc_crypt out = {0};
    unsigned int count_block = ((len + HUDI_DSC_BLOCK_SIZE - 1) / HUDI_DSC_BLOCK_SIZE);
    unsigned int i = 0;
    int left_len = len;
    int dec_size = 0;
    int dec_pos = 0;

    if(((unsigned int)input & 0x1F) || ((unsigned int)output & 0x1F))
    {
        for(i = 0; i < count_block; i++)
        {
            if(left_len < HUDI_DSC_BLOCK_SIZE)
            {
                dec_size = left_len;
            }
            else
            {
                dec_size = HUDI_DSC_BLOCK_SIZE;
            }

            out.input = inst->dsc_buf;
            out.size = dec_size;
            out.output = inst->dsc_buf;
            memcpy(inst->dsc_buf, input + dec_pos, dec_size);    
            ioctl(inst->fd, mode, &out); 
            memcpy(output + dec_pos, inst->dsc_buf, dec_size);
        
            left_len -= dec_size;
            dec_pos += dec_size;
        }
    }
    else
    {   
        vPortCacheFlush(input, len);
        vPortCacheFlush(output, len);
        vPortCacheInvalidate(output, len);
    
        out.input = input;
        out.size = len;
        out.output = output;
        ioctl(inst->fd, mode, &out);
    }
    
    return 0;
}
#else
#include <sys/mman.h>

static int _hudi_dsc_buf_alloc(hudi_dsc_instance_t *inst)
{
    inst->dsc_buf = (unsigned char*)mmap(NULL, inst->config.dsc_buf_len, PROT_READ | PROT_WRITE, MAP_SHARED, inst->fd, 0);
    if (!inst->dsc_buf)
    {      
        return -1;
    }
    
    return 0;
}

static void _hudi_dsc_buf_free(hudi_dsc_instance_t *inst)
{
    if(inst->dsc_buf)
    {
        munmap(inst->dsc_buf, inst->config.dsc_buf_len);
        inst->dsc_buf = NULL;
    }
}

static int _hudi_dsc_common_crypt(hudi_dsc_instance_t* inst, unsigned char *input, unsigned char *output, unsigned int len, int mode)   
{
    struct dsc_crypt out = {0};
    unsigned int count_block = ((len + HUDI_DSC_BLOCK_SIZE - 1) / HUDI_DSC_BLOCK_SIZE);
    int i = 0;
    int left_len = len;
    int dec_size = 0;
    int dec_pos = 0;

    for(i = 0; i < count_block; i++)
    {
        if(left_len < HUDI_DSC_BLOCK_SIZE)
        {
            dec_size = left_len;
        }
        else
        {
            dec_size = HUDI_DSC_BLOCK_SIZE;
        }

        out.input = inst->dsc_buf;
        out.size = dec_size;
        out.output = inst->dsc_buf;
        memcpy(inst->dsc_buf, input + dec_pos, dec_size);    
        ioctl(inst->fd, mode, &out); 
        memcpy(output + dec_pos, inst->dsc_buf, dec_size);
    
        left_len -= dec_size;
        dec_pos += dec_size;
    }

    return 0;
}
#endif

int hudi_dsc_key_set(hudi_handle handle, unsigned char *key, unsigned int key_len, unsigned char* iv, unsigned iv_len)
{
    hudi_dsc_instance_t *inst = (hudi_dsc_instance_t *)handle;
    struct dsc_algo_params algo_params;
    int ret = 0;

    if (!inst)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid dsc keyset parameters\n");
        return -1;
    }

    algo_params.algo_type = inst->config.algo_type;
    algo_params.crypto_mode = inst->config.crypto_mode;
    algo_params.chaining_mode = inst->config.chaining_mode;
    algo_params.residue_mode = inst->config.residue_mode;
    algo_params.key.buffer = key;
    algo_params.key.size = key_len;
    algo_params.iv.buffer = iv;
    algo_params.iv.size = iv_len;

    if (ioctl(inst->fd, DSC_CONFIG, &algo_params) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Set dsc config fail\n");
        return -1;
    }

    return 0;
}

int hudi_dsc_crypt(hudi_handle *handle, unsigned char *input, unsigned char *output, unsigned int len)
{
    hudi_dsc_instance_t *inst = (hudi_dsc_instance_t *)handle;
    int ret = 0;
    
    if (!inst || !input || !output)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid dsc encrypt parameters\n");
        return -1;
    }

    switch (inst->config.crypto_mode)
    {
        case DSC_DECRYPT:
            ret = _hudi_dsc_common_crypt(inst, input, output, len, DSC_DO_DECRYPT);       
            break;
        case DSC_ENCRYPT:
            ret = _hudi_dsc_common_crypt(inst, input, output, len, DSC_DO_ENCRYPT);       
            break;
        default:
            ret = -1;
            break;
    }
    
    return ret;
}

int hudi_dsc_open(hudi_handle *handle, hudi_dsc_config_t *config)
{
    hudi_dsc_instance_t *inst = NULL;

    if (!handle || !config)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid dsc parameters\n");
        return -1;
    }

    inst = (hudi_dsc_instance_t *)malloc(sizeof(hudi_dsc_instance_t));
    memset(inst, 0, sizeof(hudi_dsc_instance_t));

    memcpy(&inst->config, config, sizeof(hudi_dsc_config_t));
    
    inst->fd = open(HUDI_DSC_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open dsc fail\n");
        free(inst);
        return -1;
    }

    if (_hudi_dsc_buf_alloc(inst) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Alloc dsc buf fail\n");
        close(inst->fd);
        free(inst);
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    return 0;
}

int hudi_dsc_close(hudi_handle handle)
{
    hudi_dsc_instance_t *inst = (hudi_dsc_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "DSC not open\n");
        return -1;
    }

    _hudi_dsc_buf_free(inst);
    close(inst->fd);

    memset(inst, 0, sizeof(hudi_dsc_instance_t));
    free(inst);

    return 0;
}
