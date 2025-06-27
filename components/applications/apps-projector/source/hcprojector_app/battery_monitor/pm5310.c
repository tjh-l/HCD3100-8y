#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include "com_api.h"
#include "pm5310.h"
#include "app_log.h"

#ifdef __HCRTOS__
#include "nuttx/i2c/i2c_master.h"
#include <hcuapi/i2c-master.h>
#include "sys/_intsup.h"
#include "sys/_types.h"
#include <kernel/module.h>
#include <string.h>
#include <kernel/elog.h>
#include <linux/slab.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <nuttx/fs/fs.h>
#else 
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#endif 

struct pm5310_charger 	*pm5310;
struct pm5310_charger {
    unsigned int 		addr;
    const char 		*i2c_devpath;
};

#ifdef __HCRTOS__
/* 
 * i2c ioctl transfer msg need complete timing for i2c read  
 * */
static int I2C_Read(struct pm5310_charger *data, unsigned char *writebuf, int writelen,
            unsigned char *readbuf, int readlen)
{
    int ret;
    int fd;

    fd = open(data->i2c_devpath, O_RDWR);
    if (fd < 0){
        return -1;
    }

    struct i2c_transfer_s xfer_read;
    struct i2c_msg_s i2c_msg_read[2] = { 0 };

    i2c_msg_read[0].addr = (uint8_t)data->addr;
    i2c_msg_read[0].flags = 0x0;
    i2c_msg_read[0].buffer = writebuf;
    i2c_msg_read[0].length = writelen;

    i2c_msg_read[1].addr = (uint8_t)data->addr;
    i2c_msg_read[1].flags = 0x1;
    i2c_msg_read[1].buffer = readbuf;
    i2c_msg_read[1].length = readlen;

    xfer_read.msgv = i2c_msg_read;
    xfer_read.msgc = 2;

    ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);
    close(fd);
    return ret;
}

static int I2C_Write(struct pm5310_charger *data, unsigned char *writebuf, int writelen)
{
    int ret;
    int fd;

    fd = open(data->i2c_devpath, O_RDWR);
    if (fd < 0)
        return -1;

    struct i2c_transfer_s xfer_read;
    struct i2c_msg_s i2c_msg_read[1] = { 0 };

    i2c_msg_read[0].addr = (uint8_t)data->addr;
    i2c_msg_read[0].flags = 0x0;
    i2c_msg_read[0].buffer = writebuf;
    i2c_msg_read[0].length = writelen;

    xfer_read.msgv = i2c_msg_read;
    xfer_read.msgc = 1;

    ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);

    close(fd);
    return ret;
}

//read pm5310 battery register addr
int pm5310_i2c_probe(void)
{
    int 			np;
    const char 		*status;

    np = fdt_get_node_offset_by_path("/hcrtos/pm5310");  //probe pm5310 device，np=-1，0
    if (np < 0) {
        app_log(LL_DEBUG,"NO find node pm5310 in dts\n");
        return -1;
    }

    pm5310 = kzalloc(sizeof(*pm5310), GFP_KERNEL);
    if (pm5310 == NULL)
        return -ENOMEM;
    if (!fdt_get_property_string_index(np, "status", 0, &status) &&
        !strcmp(status, "disabled"))
        return -1;
    if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&pm5310->addr))
        return -1;
    if (fdt_get_property_string_index(np, "i2c-devpath", 0, &pm5310->i2c_devpath))
        return -1;
    return 0;
}

#else 
static int I2C_Read(struct pm5310_charger *data, unsigned char *writebuf, int writelen,
            unsigned char *readbuf, int readlen)
{
    int fd = -1;
    int ret = -1;
    fd = open(data->i2c_devpath,O_RDWR);
    if(fd < 0){
       app_log(LL_ERROR,"open i2c_dev error"); 
       return ret ;
    }
    ret = ioctl(fd,I2C_TENBIT,0);
    if(ret < 0){
        app_log(LL_ERROR,"i2c dev transfer error");
        close(fd);
        return ret;
    }
    ret = ioctl(fd,I2C_SLAVE,data->addr);
    if(ret < 0){
        app_log(LL_ERROR,"i2c dev transfer error");
        close(fd);
        return ret;
    }
    ret = write(fd,writebuf,writelen);
    if(ret > 0){
        ret = read(fd,readbuf,readlen);
    }
    close(fd);
    return ret;
}

static int I2C_Write(struct pm5310_charger *data, unsigned char *writebuf, int writelen)
{
    int fd = -1;
    int ret = -1;
    fd = open(data->i2c_devpath,O_RDWR);
    if(fd < 0){
       app_log(LL_ERROR,"open i2c_dev error"); 
       return ret ;
    }
    ret = ioctl(fd,I2C_TENBIT,0);
    if(ret < 0){
        app_log(LL_ERROR,"i2c dev transfer error");
        close(fd);
        return ret;
    }
    ret = ioctl(fd,I2C_SLAVE,data->addr);
    if(ret < 0){
        app_log(LL_ERROR,"i2c dev transfer error");
        close(fd);
        return ret;
    }
    ret = write(fd,writebuf,writelen);
    close(fd);
    return ret;
}

#define PM5310_CONFIG_PATH      "/proc/device-tree/i2c-gpio/pm5310/"
char i2c_devpath[16] = {0}; 
static int pm5310_i2c_probe(void)
{
    char status[16] = {0};
    int ret = -1;
    api_dts_string_get(PM5310_CONFIG_PATH"/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        pm5310 = (struct pm5310_charger*)malloc(sizeof(struct pm5310_charger));
        pm5310->addr = api_dts_uint32_get(PM5310_CONFIG_PATH"/i2c-addr");
        if(pm5310->addr < 0){
            free(pm5310);
            return ret; 
        }
        api_dts_string_get(PM5310_CONFIG_PATH"/i2c-devpath", i2c_devpath, sizeof(i2c_devpath));
        if(!strcmp(i2c_devpath,"")){
            free(pm5310);
            return ret;
        }
        pm5310->i2c_devpath = i2c_devpath;
        ret = 0;
    }
    return ret ;
}

#endif 


/**
 * @brief 
 * 
 * @param buf_read_addr :pm5310 register address
 * @param buf_read :get data from registers
 * @param buf_read_bit :write a certain bit
 * @return int 
 */
int i2c_read_pm5310_state(unsigned char buf_read_addr, unsigned char *buf_read, unsigned char buf_read_bit)
{
    int ret=0;
    unsigned char read=0x00;
    ret=I2C_Read(pm5310, &buf_read_addr, sizeof(buf_read_addr), &read, sizeof(read));
    *buf_read=read & buf_read_bit;
    if (ret<0) {
        return API_FAILURE;
    }
    return ret;
}

/**
 * @brief 
 * 
 * @param buf_write_addr :pm5310 register address
 * @param buf_write_bit  :write a certain bit
 * @param write_data  :Data to replace, 0 or 1
 * @return int 
 */
int i2c_write_pm5310_state(unsigned char buf_write_addr, unsigned char buf_write_bit, int write_mode)
{
    int ret=0;
    unsigned char buf_read=0x00;
    unsigned char buf_write[2];
    ret=I2C_Read(pm5310, &buf_write_addr, sizeof(buf_write_addr), &buf_read, sizeof(buf_read));
    if (ret<0) {
        return API_FAILURE;
    }
    switch (write_mode) {
    case 1:
        buf_write[0] = buf_write_addr;
        buf_write[1] = buf_read | buf_write_bit;
        ret=I2C_Write(pm5310, buf_write, sizeof(buf_write));
        break;
    case 0:
        buf_write[0] = buf_write_addr;
        buf_write[1] = buf_read & buf_write_bit;
        ret=I2C_Write(pm5310, buf_write, sizeof(buf_write));
        break;
    }
    return ret;
}
