#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>

#include <hudi/hudi_com.h>
#include <hudi/hudi_dsc.h>

#include <hccast_dsc.h>
#include <hccast_log.h>

void* hccast_dsc_aes_ctr_open(int dsc_buf_len)
{
    hudi_handle dsc_hdl = NULL;
    hudi_dsc_config_t dsc_config;

    dsc_config.algo_type = DSC_ALGO_AES;
    dsc_config.chaining_mode = DSC_MODE_CTR;
    dsc_config.crypto_mode = DSC_DECRYPT;
    dsc_config.residue_mode = DSC_RESIDUE_CLEAR;
    dsc_config.dsc_buf_len = dsc_buf_len;
    hudi_dsc_open(&dsc_hdl, &dsc_config);
    
    return dsc_hdl;
}

void* hccast_dsc_aes_cbc_open(int dsc_buf_len)
{
    hudi_handle dsc_hdl = NULL;
    hudi_dsc_config_t dsc_config;

    dsc_config.algo_type = DSC_ALGO_AES;
    dsc_config.chaining_mode = DSC_MODE_CBC;
    dsc_config.crypto_mode = DSC_DECRYPT;
    dsc_config.residue_mode = DSC_RESIDUE_CLEAR;
    dsc_config.dsc_buf_len = dsc_buf_len;
    hudi_dsc_open(&dsc_hdl, &dsc_config);
    
    return dsc_hdl;
}

void hccast_dsc_ctx_destroy(void *ctx)
{
    hudi_dsc_close(ctx);
}

int hccast_dsc_decrypt(void *ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len)
{
    int ret = 0;

    if (key && iv)
    {
        if (hudi_dsc_key_set(ctx, key, 16, iv, 16) < 0)
        {
            return -1;
        }
    }
    
    if (input && output)
    {
        ret = hudi_dsc_crypt(ctx, input, output, len);
    }

    return ret;
}

int hccast_dsc_encrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len)
{
    int ret = 0;
    
    if (key && iv)
    {
        if (hudi_dsc_key_set(ctx, key, 16, iv, 16) < 0)
        {
            return -1;
        }
    }

    if (input && output)
    {
        ret = hudi_dsc_crypt(ctx, input, output, len);
    }
    
    return ret;
}
