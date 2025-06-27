#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hudi/hudi_com.h>
#include <hudi/hudi_flash.h>

static void dump_hex(const char *str, unsigned char *data, unsigned int len)
{
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
}

int flash_test_start(void)
{
    hudi_handle hdl = NULL;
    unsigned char uid[16] = {0};
    unsigned char otp[512] = {0};
    int i;

    if (0 != hudi_flash_open(&hdl))
    {
        printf("hudi flash open fail\n");
        return -1;
    }

    hudi_flash_uid_read(hdl, uid);
    dump_hex("uid", uid, 16);

    hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, otp, 32);
    dump_hex("otp", otp, 32);

    memset(otp, 0, 128);
    otp[0] = 0x02;
    otp[1] = 0x01;
    otp[2] = 0x02;
    otp[22] = 0x22;
    otp[100] = 0xa0;
    otp[110] = 0xb0;
    hudi_flash_otp_write(hdl, HUDI_FLASH_OTP_REG3, 14, otp, 128);

    hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, otp, 256);
    dump_hex("otp3", otp, 256);

    hudi_flash_close(hdl);

    return 0;
}
