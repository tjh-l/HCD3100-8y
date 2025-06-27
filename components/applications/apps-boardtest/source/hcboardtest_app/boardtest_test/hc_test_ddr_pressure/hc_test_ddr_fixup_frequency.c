#include "app_config.h"
#include "com_api.h"
#include "boardtest_module.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <nuttx/mtd/mtd.h>
#include <hcuapi/standby.h>
#include <cpu_func.h>

#define	DDR_DIR	"/hichip_boardtest/ddrsetting/"
#define BLOCK_SIZE	64*1024
#define DDRADDR_LSEEK	2*1024
struct ddr_setting {
    int frequency;
    const char *setting;
};

static struct ddr_setting setting[] = {
    {800, "hc15xx_ddr2_128M_800MHz.abs"},
    {750, "hc15xx_ddr2_128M_750MHz.abs"},
    {660, "hc15xx_ddr2_128M_660MHz.abs"}, 
    {600, "hc15xx_ddr2_128M_600MHz.abs"}
};

static uint32_t hc_test_get_current_ddr_frequency(void)
{
    uint32_t frequency = 0, ic;
    frequency = REG32_GET_FIELD2(0xb8800070, 4, 3);
    switch (frequency) {
    case 0:
        frequency = 800;
        break;
    case 1:
        frequency = 1066;
        break;
    case 2:
        frequency = 1333;
        break;
    case 3:
        frequency = 1600;
        break;
    case 4:
        frequency = 600;
        break;
    default:
        break;
    }

    ic = REG8_READ(0xb8800003);
    if (REG16_GET_BIT(0xb880048a, BIT15)) {
        if (ic == 0x15) 
            frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 12;
        else if (ic == 0x16)
            frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 24;
    }
	
    return frequency;
}

static int hc_test_get_next_ddr_setting(char *path, int len)
{
    int freq;
    struct stat st;
    char *mnt_point;
	
    mnt_point = api_get_ad_mount_add();
    freq = hc_test_get_current_ddr_frequency();

    for (int i = 0; i < ARRAY_SIZE(setting); i++) {
        if (setting[i].frequency >= freq)
            continue;

        memset(path, 0, len);
        strcat(path, "/media/");
        strcat(path, mnt_point);
        strcat(path, DDR_DIR);
        strcat(path, setting[i].setting);
        if (stat(path, &st) != -1)
            return (int)st.st_size;
    }

    return -1;
}

static int hc_test_reduce_ddr_frequency(void)
{
    int fd_mtd = -1;
    int fd_ddr = -1;
    int sz_ddr = 0;
    char path[128];
    char *buf = NULL;
    int rc = 0;

    sz_ddr = hc_test_get_next_ddr_setting(path, sizeof(path));
    if (sz_ddr <= 0) {
        printf("Lower DDR frequency required!!\r\n");
        return -1;
    }

    fd_ddr = open(path, O_SYNC | O_RDONLY);
    if (fd_ddr < 0) {
        printf("Lower DDR frequency required!!\r\n");
        return -1;
    }

    fd_mtd = open("/dev/mtdblock1", O_SYNC | O_RDWR);
    if (fd_mtd < 0) {
        printf("Error: open /dev/mtdblock1 failed \n");
        rc = -1;
        goto err_ret;
    }

    buf = (char *)malloc(BLOCK_SIZE);
    if (!buf) {
        printf("Error: malloc buf failed \n");
        rc = -1;
        goto err_ret;
    }

    if(read(fd_mtd, buf, BLOCK_SIZE) < 0){
        printf("Error: read fd_mtd failed \n");
        rc = -1;
        goto err_ret;
    }
	
    lseek(fd_ddr, DDRADDR_LSEEK, SEEK_SET);

    if(read(fd_ddr, buf + DDRADDR_LSEEK, sz_ddr - DDRADDR_LSEEK) < 0){
        printf("Error: read fd_ddr failed \n");
        rc = -1;
        goto err_ret;
    }
	
    lseek(fd_mtd, (off_t)0, SEEK_SET);
	
    if(write(fd_mtd, buf, BLOCK_SIZE) < 0){
        printf("Error: write fd_mtd failed \n");
        rc = -1;
        goto err_ret;
    }

    rc = 0;

err_ret:
    if (fd_mtd >= 0)
        close(fd_mtd);

    if (fd_ddr)
        close(fd_ddr);
	
    if (buf){
        free(buf);
        buf = NULL;
    }
    return rc;
}

void hc_test_fixup_ddr_setting(void)
{
    standby_bootup_mode_e bootmode = STANDBY_BOOTUP_COLD_BOOT;
    uint32_t frequency = 0;
    char str[16] = { 0 };
    int fd;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0) {
        printf("Open /dev/standby failed, assume cold boot!\n");
    } else {
        ioctl(fd, STANDBY_GET_BOOTUP_MODE, &bootmode);
        close(fd);
    }

    frequency = hc_test_get_current_ddr_frequency();
    if (bootmode != STANDBY_BOOTUP_COLD_BOOT) {
        if (hc_test_reduce_ddr_frequency() == 0) {
            REG8_WRITE(0xb8818a70, 0x00); /* mark as cold boot */
            reset();
        }
    }

    sprintf(str, "%ldMHz", frequency);
    write_boardtest_detail_instantly(BOARDTEST_DDR_STRESS_TEST, str);
}

