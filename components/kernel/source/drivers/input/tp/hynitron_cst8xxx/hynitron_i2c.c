/**
 *Name        : cst0xx_i2c.c
 *Author      : gary
 *Version     : V1.0
 *Create      : 2018-1-23
 *Copyright   : zxzz
 */


#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include "hynitron_core.h"

/*****************************************************************************/
static DEFINE_MUTEX(i2c_rw_access);

static int i2c_master_send(struct hynitron_ts_data *client, unsigned char *writebuf, int writelen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;
	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)client->addr,
			.flags = 0,
			.length = writelen,
			.buffer = writebuf,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(client->i2c_devpath, O_RDWR);

	while (retry--) {
		if (writelen > 0) {
			ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
			if (ret < 0) {
				HYN_ERROR("error.retry = %d\n", retry);
				continue;
			}
		}
		break;
	}

	close(fd);

	return ret;
}

static int i2c_master_recv(struct hynitron_ts_data *client, unsigned char *readbuf, int readlen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)client->addr,
			.flags = I2C_M_RD,
			.length = readlen,
			.buffer = readbuf,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(client->i2c_devpath, O_RDWR);

	while (retry--) {
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if (ret < 0) {
			HYN_ERROR("error.retry = %d\n", retry);
			continue;
		}
		break;
	}

	close(fd);

	return ret;
}

int hyn_i2c_read(struct hynitron_ts_data *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret = -1;

	if (client == NULL) {
		HYN_ERROR("[IIC][%s]hynitron_ts_data==NULL!", __func__);
		return -1;
	}

	// client->addr = client->addr & I2C_MASK_FLAG;
	ret = i2c_master_send(client, writebuf, writelen);
	if (ret < 0) {
		HYN_ERROR("i2c_master_send error\n");
	}

	// client->addr = (client->addr & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_RS_FLAG;
	ret = i2c_master_recv(client, readbuf, readlen);
	if (ret < 0) {
		HYN_ERROR("i2c_master_recv i2c read error.\n");
		return ret;
	}

	return ret;
}

/*
 *
 */
int hyn_i2c_write(struct hynitron_ts_data *client, char *writebuf, int writelen)
{
	int ret = -1;

	if (client == NULL) {
		HYN_ERROR("[IIC][%s]hynitron_ts_data==NULL!", __func__);
		return -1;
	}
	// client->addr = client->addr & I2C_MASK_FLAG;
	ret = i2c_master_send(client, writebuf, writelen);
	if (ret < 0) {
		HYN_ERROR("i2c_master_send error\n");
	}

	return ret;
}

/*
 *
 */
int hyn_i2c_write_byte(struct hynitron_ts_data *client, u8 regaddr, u8 regvalue)
{
    u8 buf[2] = {0};

    buf[0] = regaddr;
    buf[1] = regvalue;
    return hyn_i2c_write(client, buf, sizeof(buf));
}
/*
 *
 */
int hyn_i2c_read_byte(struct hynitron_ts_data *client, u8 regaddr, u8 *regvalue)
{
    return hyn_i2c_read(client, &regaddr, 1, regvalue, 1);
}

/*****************************************************************/
/*
 *
 */
int hyn_i2c_write_bytes(unsigned short reg, unsigned char *buf, unsigned short len, unsigned char reg_len)
{
	int ret;
	unsigned char mbuf[600];
	if (reg_len == 1) {
		mbuf[0] = reg;
		memcpy(mbuf + 1, buf, len);
	} else {
		mbuf[0] = reg >> 8;
		mbuf[1] = reg;
		memcpy(mbuf + 2, buf, len);
	}

	ret = hyn_i2c_write(hyn_ts_data, mbuf, len + reg_len);
	if (ret < 0) {
		HYN_ERROR("%s i2c write error.\n", __func__);
	}
	return ret;
}
/*
 *
 */
int hyn_i2c_read_bytes(unsigned short reg, unsigned char *buf, unsigned short len, unsigned char reg_len)
{
	int ret;
	unsigned char reg_buf[2];
	if (reg_len == 1) {
		reg_buf[0] = reg;
	} else {
		reg_buf[0] = reg >> 8;
		reg_buf[1] = reg;
	}

	ret = hyn_i2c_read(hyn_ts_data, reg_buf, reg_len, buf, len);
	if (ret < 0) {
		HYN_ERROR("f%s: i2c read error.\n", __func__);
	}

	return ret;
}

/*****************************************************************/

#ifdef HIGH_SPEED_IIC_TRANSFER
 int cst3xx_i2c_read(struct hynitron_ts_data *client, unsigned char *buf, int len) 
{ 
	struct i2c_msg msg; 
	int ret = -1; 
	int retries = 0; 
	
	msg.flags |= I2C_M_RD; 
	msg.addr   = client->addr;
	msg.len    = len; 
	msg.buf    = buf;	

	while (retries < 2) { 
		ret = i2c_transfer(client->adapter, &msg, 1); 
		if(ret == 1)
			break; 
		retries++; 
	} 
	
	return ret; 
} 


/*******************************************************
Function:
    read data from register.
Input:
    buf: first two byte is register addr, then read data store into buf
    len: length of data that to read
Output:
    success: number of messages
    fail:	negative errno
*******************************************************/
int cst3xx_i2c_read_register(struct hynitron_ts_data *client, unsigned char *buf, int len)
{ 
	struct i2c_transfer_s xfer;
	struct i2c_msg msgs[2]; 
	int ret = -1; 
	int retries = 0; 
	
	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;  
	msgs[0].len   = 2;
	msgs[0].buf   = buf;

	msgs[1].flags  = 1;
	msgs[1].addr   = client->addr; 
	msgs[1].len    = len; 
	msgs[1].buf    = buf;

	xfer.msgv = msgs;
	xfer.msgc = 2;

	fd = open(client->i2c_devpath, O_RDWR);

	while (retries < 2) { 
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if(ret == 2)
			break; 
		retries++; 
	} 

	close(fd);
	
	return ret; 
}

int cst3xx_i2c_write(struct hynitron_ts_data *client, unsigned char *buf, int len)
{ 
	struct i2c_transfer_s xfer;
	struct i2c_msg msg; 
	int ret = -1; 
	int retries = 0;

	msg.flags = 0; 
	msg.addr  = client->addr; 
	msg.len   = len; 
	msg.buf   = buf;		  

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(client->i2c_devpath, O_RDWR);
	  
	while (retries < 2) { 
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if(ret == 1)
			break; 
		retries++; 
	} 	
	
	close(fd);

	return ret; 
}

#else
int cst3xx_i2c_read(struct hynitron_ts_data *client, unsigned char *buf, int len) 
{ 
	int ret = -1; 
	int retries = 0; 

	while (retries < 2) { 
		ret = i2c_master_recv(client, buf, len); 
		if(ret<=0) 
		    retries++;
        else
            break; 
	} 
	
	return ret; 
} 

int cst3xx_i2c_write(struct hynitron_ts_data *client, unsigned char *buf, int len) 
{ 
	int ret = -1; 
	int retries = 0; 

	while (retries < 2) { 
		ret = i2c_master_send(client, buf, len); 
		if(ret<=0) 
		    retries++;
        else
            break; 
	} 
	
	return ret; 
}

int cst3xx_i2c_read_register(struct hynitron_ts_data *client, unsigned char *buf, int len) 
{ 
	int ret = -1; 
    
    ret = cst3xx_i2c_write(client, buf, 2);

    ret = cst3xx_i2c_read(client, buf, len);
	
    return ret; 
} 

#endif

