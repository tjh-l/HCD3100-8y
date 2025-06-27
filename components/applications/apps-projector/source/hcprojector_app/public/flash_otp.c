#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flash_otp.h"
#include "app_log.h"

#ifdef HUDI_FLASH_SUPPORT
#include <hudi/hudi_com.h>
#include <hudi/hudi_flash.h>
typedef struct otp_data_t_{
  #ifdef CVBSIN_TRAINING_SUPPORT
    uint8_t cvbs_training_value;
    #define CVBS_TRAINING_VALUE_LEN 1
  #else
    #define CVBS_TRAINING_VALUE_LEN 0
  #endif
    uint8_t reserved[256 - CVBS_TRAINING_VALUE_LEN];
} otp_data_t;

//#define OTP_DEBUG
static otp_data_t otp_data = {0};

static void dump_hex(const char *str, unsigned char *data, unsigned int len)
{
#ifndef OTP_DEBUG
    (void)str;
    (void)data;
    (void)len;
    return;
#else
    int i;
    printf("%s:", str);
    for (i = 0; i < len; i ++)
    {
        if (i % 32 == 0)
        {
            printf("\n");
        }
        printf("%.2x ", data[i]);
    }
    printf("\n");
#endif    
}

void flash_otp_data_init(void)
{
    static int m_flash_otp_init = 0;
    hudi_handle hdl = NULL;
    unsigned char uid[16] = {0};
    unsigned int uid_len = 0;

    if (m_flash_otp_init)
        return;
    m_flash_otp_init = 1;

    if (0 != hudi_flash_open(&hdl, HUDI_FLASH_TYPE_NOR))
    {
        printf("hudi flash open fail\n");
        return;
    }else{
    	printf("hudi flash open success\n");
    }

    hudi_flash_uid_read(hdl, uid, &uid_len);
    #if 0
    dump_hex("uid", uid, 16);
    #endif
    hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, (unsigned char*)&otp_data, sizeof(otp_data_t));
    #if 0 
    dump_hex("otp_data", (unsigned char*)&otp_data, sizeof(otp_data_t));
    #endif

    hudi_flash_close(hdl);
}

#ifdef CVBSIN_TRAINING_SUPPORT
uint8_t cvbs_training_value_get(void)
{
	return otp_data.cvbs_training_value;
}

void cvbs_training_value_set(uint8_t val)
{
	otp_data.cvbs_training_value = val;
}
#endif

void flash_otp_data_saved(void)
{
    hudi_handle hdl = NULL;

    if (0 != hudi_flash_open(&hdl, HUDI_FLASH_TYPE_NOR))
    {
        printf("hudi flash open fail\n");
        return;
    }else{
    	printf("hudi flash open success\n");
    }

    hudi_flash_otp_write(hdl, HUDI_FLASH_OTP_REG3, 0, (unsigned char*)&otp_data, sizeof(otp_data_t));

    //do not read at once???
    // otp_data_t otp_data_read = {0};
    // hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, (unsigned char*)&otp_data_read, sizeof(otp_data_t));
    // dump_hex("otp_data_read", (unsigned char*)&otp_data_read, sizeof(otp_data_t));

    hudi_flash_close(hdl);
}
#endif
