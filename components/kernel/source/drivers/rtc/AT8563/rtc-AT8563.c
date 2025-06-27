/*
 * AnalogTek AT8563 RTC driver
 *
 * Copyright (C) 2013 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * based on rtc-AT8563
 * Copyright (C) 2010 ROCKCHIP, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <nuttx/fs/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>

#include <linux/module.h>
#include <hcuapi/i2c-master.h>
#include <linux/bcd.h>
#include <uapi/linux/rtc.h>
#include <linux/delay.h>
#include <errno.h>
#include <linux/of.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/gpio.h>
#include <linux/time64.h>
#include <linux/math64.h>
#include <linux/rtc.h>

#define AT8563_I2C_NAME 	        "AT8563"

#define AT8563_CTL1		0x00
#define AT8563_CTL1_TEST	BIT(7)
#define AT8563_CTL1_STOP	BIT(5)
#define AT8563_CTL1_TESTC	BIT(3)

#define AT8563_CTL2		0x01
#define AT8563_CTL2_TI_TP	BIT(4)
#define AT8563_CTL2_AF		BIT(3)
#define AT8563_CTL2_TF		BIT(2)
#define AT8563_CTL2_AIE	BIT(1)
#define AT8563_CTL2_TIE	BIT(0)

#define AT8563_SEC		0x02
#define AT8563_SEC_VL		BIT(7)
#define AT8563_SEC_MASK	0x7f

#define AT8563_MIN		0x03
#define AT8563_MIN_MASK	0x7f

#define AT8563_HOUR		0x04
#define AT8563_HOUR_MASK	0x3f

#define AT8563_DAY		0x05
#define AT8563_DAY_MASK	0x3f

#define AT8563_WEEKDAY		0x06
#define AT8563_WEEKDAY_MASK	0x07

#define AT8563_MONTH		0x07
#define AT8563_MONTH_CENTURY	BIT(7)
#define AT8563_MONTH_MASK	0x1f
#define AT8563_CENTURY_MASK	0x80

#define AT8563_YEAR		0x08

#define AT8563_ALM_MIN		0x09
#define AT8563_ALM_HOUR	0x0a
#define AT8563_ALM_DAY		0x0b
#define AT8563_ALM_WEEK	0x0c

/* Each alarm check can be disabled by setting this bit in the register */
#define AT8563_ALM_BIT_DISABLE	BIT(7)

#define AT8563_CLKOUT		0x0d
#define AT8563_CLKOUT_ENABLE	BIT(7)
#define AT8563_CLKOUT_32768	0
#define AT8563_CLKOUT_1024	1
#define AT8563_CLKOUT_32	2
#define AT8563_CLKOUT_1	3
#define AT8563_CLKOUT_MASK	3

#define AT8563_TMR_CTL		0x0e
#define AT8563_TMR_CTL_ENABLE	BIT(7)
#define AT8563_TMR_CTL_4096	0
#define AT8563_TMR_CTL_64	1
#define AT8563_TMR_CTL_1	2
#define AT8563_TMR_CTL_1_60	3
#define AT8563_TMR_CTL_MASK	3

#define AT8563_TMR_CNT		0x0f

//#define DEBUG
#ifdef DEBUG
#define rtc_dbg(format, args...)                                               \
	do {                                                                   \
		printf(format, ##args);                                        \
	} while (0)
#else
#define rtc_dbg(format, args...)                                               \
	do {                                                                   \
	} while (0)
#endif /* DEBUG */

struct AT8563 {
	bool			valid;
	const char 		*i2c_devpath;
	unsigned int 		addr;
	struct work_s           work;
	struct mutex 		ops_lock;
	unsigned int 		irq;
};

static int i2c_smbus_read_byte_data(struct AT8563 *data, u8 reg_addr)
{
	int fd;
	int ret;
	unsigned char readbuf[1];
	unsigned char writebuf[1];
	writebuf[0] = reg_addr;

	fd = open(data->i2c_devpath, O_RDWR);
	if (fd < 0)
		return -1;

	struct i2c_transfer_s xfer_read;
	struct i2c_msg_s i2c_msg_read[2] = { 0 };

	i2c_msg_read[0].addr = (uint8_t)data->addr;
	i2c_msg_read[0].flags = 0x0;
	i2c_msg_read[0].buffer = writebuf;
	i2c_msg_read[0].length = 1;

	i2c_msg_read[1].addr = (uint8_t)data->addr;
	i2c_msg_read[1].flags = 0x1;
	i2c_msg_read[1].buffer = readbuf;
	i2c_msg_read[1].length = 1;

	xfer_read.msgv = i2c_msg_read;
	xfer_read.msgc = 2;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);
	if (ret < 0)
		rtc_dbg("%s: i2c read error.\n", __func__);

	close(fd);

	return (ret < 0) ? ret : readbuf[0];
}

static int i2c_smbus_write_byte_data(struct AT8563 *data, u8 reg_addr, u8 wdata)
{
	int fd;
	int ret;
	unsigned char writebuf[2];
	writebuf[0] = reg_addr;
	writebuf[1] = wdata;

	fd = open(data->i2c_devpath, O_RDWR);
	if (fd < 0)
		return -1;

	struct i2c_transfer_s xfer_write;
	struct i2c_msg_s i2c_msg_write;

	i2c_msg_write.addr = (uint8_t)data->addr;
	i2c_msg_write.flags = 0x0;
	i2c_msg_write.buffer = writebuf;
	i2c_msg_write.length = 2;

	xfer_write.msgv = &i2c_msg_write;
	xfer_write.msgc = 1;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
	if (ret < 0)
		rtc_dbg("%s:i2c write byte error.\n", __func__);

	close(fd);

	return ret;
}

static int i2c_smbus_write_i2c_block_data(struct AT8563 *data, u8 reg_addr,
					  int writelen,
					  unsigned char *writedata)
{
	int fd;
	int ret;
	unsigned char writebuf[1 + writelen];
	writebuf[0] = reg_addr;

	memcpy((void *)&writebuf[1], (void *)writedata, writelen);

	fd = open(data->i2c_devpath, O_RDWR);
	if (fd < 0)
		return -1;

	struct i2c_transfer_s xfer_write;
	struct i2c_msg_s i2c_msg_write;

	i2c_msg_write.addr = (uint8_t)data->addr;
	i2c_msg_write.flags = 0x0;
	i2c_msg_write.buffer = writebuf;
	i2c_msg_write.length = 1 + writelen;

	xfer_write.msgv = &i2c_msg_write;
	xfer_write.msgc = 1;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
	if (ret < 0)
		rtc_dbg("%s:i2c write error.\n", __func__);

	close(fd);

	return ret;
}
static int i2c_smbus_read_i2c_block_data(struct AT8563 *data, u8 reg_addr,
					 int readlen, unsigned char *readbuf)
{
	int fd;
	int ret;
	unsigned char writebuf[1];
	writebuf[0] = reg_addr;

	fd = open(data->i2c_devpath, O_RDWR);
	if (fd < 0)
		return -1;

	struct i2c_transfer_s xfer_read;
	struct i2c_msg_s i2c_msg_read[2] = { 0 };

	i2c_msg_read[0].addr = (uint8_t)data->addr;
	i2c_msg_read[0].flags = 0x0;
	i2c_msg_read[0].buffer = writebuf;
	i2c_msg_read[0].length = 1;

	i2c_msg_read[1].addr = (uint8_t)data->addr;
	i2c_msg_read[1].flags = 0x1;
	i2c_msg_read[1].buffer = readbuf;
	i2c_msg_read[1].length = readlen;

	xfer_read.msgv = i2c_msg_read;
	xfer_read.msgc = 2;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);
	if (ret < 0)
		rtc_dbg("f%s: i2c read error.\n", __func__);

	close(fd);

	return ret;
}
/*
 * RTC handling
 */
static int AT8563_rtc_read_time(struct AT8563 *data, struct rtc_time *tm)
{
	u8 buf[7];
	int ret;

	//if (!AT8563->valid) {
	//dev_warn(&client->dev, "no valid clock/calendar values available\n");
	//return -EPERM;
	//}

	ret = i2c_smbus_read_i2c_block_data(data, AT8563_SEC, 7, buf);

	tm->tm_sec = bcd2bin((u32)buf[0] & AT8563_SEC_MASK);
	tm->tm_min = bcd2bin((u32)buf[1] & AT8563_MIN_MASK);
	tm->tm_hour = bcd2bin((u32)buf[2] & AT8563_HOUR_MASK);
	tm->tm_mday = bcd2bin((u32)buf[3] & AT8563_DAY_MASK);
	tm->tm_wday = bcd2bin((u32)buf[4] & AT8563_WEEKDAY_MASK); /* 0 = Sun */
	tm->tm_mon = bcd2bin((u32)buf[5] & AT8563_MONTH_MASK) - 1; /* 0 = Jan */
	//tm->tm_mon = bcd2bin((u32)buf[5] & AT8563_MONTH_MASK); /* 0 = Jan */
	//century=bcd2bin(buf[5] & AT8563_CENTURY_MASK) >> 7 ;
	//printk("%s:----niotong----century=%d\n",__func__,century);
	tm->tm_year = bcd2bin((u32)buf[6]) + 100;

	rtc_dbg("%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__, tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday,
		tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_wday);
	return 0;
}

static int AT8563_rtc_set_time(struct AT8563 *data, struct rtc_time *tm)
{
	u8 buf[7];
	int ret;

	/* Years >= 2100 are to far in the future, 19XX is to early */
	if (tm->tm_year < 100 || tm->tm_year >= 200)
		//if (tm->tm_year >= 200)
		return -EINVAL;

	buf[0] = bin2bcd(tm->tm_sec);
	buf[1] = bin2bcd(tm->tm_min);
	buf[2] = bin2bcd(tm->tm_hour);
	buf[3] = bin2bcd(tm->tm_mday);
	buf[4] = bin2bcd(tm->tm_wday);
	buf[5] = bin2bcd(tm->tm_mon + 1);
	//buf[5] = bin2bcd(tm->tm_mon);

	/*
	 * While the AT8563 has a century flag in the month register,
	 * it does not seem to carry it over a subsequent write/read.
	 * So we'll limit ourself to 100 years, starting at 2000 for now.
	 */
#if 0
	printf("%s: year=%d, wday=%d\n", __func__, tm->tm_year, tm->tm_wday);
	if (tm->tm_year >= 100) {
		buf[5] = buf[5] & 0x7f; //set bit7 0
		printk("----year>=100---niotong-----%x---\n", buf[5]);
		buf[6] = bin2bcd(tm->tm_year - 100);
	} else {
		//buf[5] = buf[5] | 0x80;      //set bit8 1
		printk("----year<100---niotong-----%x---\n", buf[5]);
		buf[6] = bin2bcd(tm->tm_year);
	}
#endif
	buf[5] = buf[5] & 0x7f; //set bit7 0 : 21century
	buf[6] = bin2bcd(tm->tm_year - 100);

	rtc_dbg("%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__, tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday,
		tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_wday);
	/*
	 * CTL1 only contains TEST-mode bits apart from stop,
	 * so no need to read the value first
	 */
	ret = i2c_smbus_write_byte_data(data, AT8563_CTL1, AT8563_CTL1_STOP);
	if (ret < 0)
		return ret;

	ret = i2c_smbus_write_i2c_block_data(data, AT8563_SEC, 7, buf);
	if (ret < 0)
		return ret;

	ret = i2c_smbus_write_byte_data(data, AT8563_CTL1, 0);
	if (ret < 0)
		return ret;

	//AT8563->valid = true;

	return 0;
}

static int AT8563_rtc_alarm_irq_enable(struct AT8563 *data,
				       unsigned int enabled)
{
	int readdata;

	rtc_dbg("%s\n", __func__);
	readdata = i2c_smbus_read_byte_data(data, AT8563_CTL2);
	if (readdata < 0)
		return readdata;

	if (enabled)
		readdata |= AT8563_CTL2_AIE;
	else
		readdata &= ~AT8563_CTL2_AIE;

	return i2c_smbus_write_byte_data(data, AT8563_CTL2, readdata);
};

static int AT8563_rtc_read_alarm(struct AT8563 *data, struct rtc_wkalrm *alm)
{
	struct rtc_time *alm_tm = &alm->time;
	u8 buf[4];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(data, AT8563_ALM_MIN, 4, buf);
	if (ret < 0)
		return ret;

	/* The alarm only has a minute accuracy */
	alm_tm->tm_sec = -1;

	alm_tm->tm_min = ((u32)buf[0] & AT8563_ALM_BIT_DISABLE) ?
				 (u32)-1 :
				 bcd2bin((u32)buf[0] & AT8563_MIN_MASK);
	alm_tm->tm_hour = ((u32)buf[1] & AT8563_ALM_BIT_DISABLE) ?
				  (u32)-1 :
				  bcd2bin((u32)buf[1] & AT8563_HOUR_MASK);
	alm_tm->tm_mday = ((u32)buf[2] & AT8563_ALM_BIT_DISABLE) ?
				  (u32)-1 :
				  bcd2bin((u32)buf[2] & AT8563_DAY_MASK);
	alm_tm->tm_wday = ((u32)buf[3] & AT8563_ALM_BIT_DISABLE) ?
				  (u32)-1 :
				  bcd2bin((u32)buf[3] & AT8563_WEEKDAY_MASK);

	alm_tm->tm_mon = -1;
	alm_tm->tm_year = -1;

	ret = i2c_smbus_read_byte_data(data, AT8563_CTL2);
	if (ret < 0)
		return ret;
	if (ret & AT8563_CTL2_AIE)
		alm->enabled = 1;

	return 0;
}

static int AT8563_rtc_set_alarm(struct AT8563 *data, struct rtc_wkalrm *alm)
{
	struct rtc_time *alm_tm = &alm->time;
	u8 buf[4];
	int ret;

	/*
	 * The alarm has no seconds so deal with it
	 */
	if (alm_tm->tm_sec) {
		alm_tm->tm_sec = 0;
		alm_tm->tm_min++;
		if (alm_tm->tm_min >= 60) {
			alm_tm->tm_min = 0;
			alm_tm->tm_hour++;
			if (alm_tm->tm_hour >= 24) {
				alm_tm->tm_hour = 0;
				alm_tm->tm_mday++;
				if (alm_tm->tm_mday > 31)
					alm_tm->tm_mday = 0;
			}
		}
	}

	rtc_dbg("%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__func__, alm_tm->tm_sec, alm_tm->tm_min, alm_tm->tm_hour,
		alm_tm->tm_mday, alm_tm->tm_mon + 1, alm_tm->tm_year + 1900,
		alm_tm->tm_wday);

	ret = i2c_smbus_read_byte_data(data, AT8563_CTL2);
	if (ret < 0)
		return ret;

	ret &= ~AT8563_CTL2_AIE;

	ret = i2c_smbus_write_byte_data(data, AT8563_CTL2, ret);
	if (ret < 0)
		return ret;

	buf[0] = (alm_tm->tm_min < 60 && alm_tm->tm_min >= 0) ?
			 bin2bcd((u32)alm_tm->tm_min) :
			 AT8563_ALM_BIT_DISABLE;

	buf[1] = (alm_tm->tm_hour < 24 && alm_tm->tm_hour >= 0) ?
			 bin2bcd((u32)alm_tm->tm_hour) :
			 AT8563_ALM_BIT_DISABLE;

	buf[2] = (alm_tm->tm_mday <= 31 && alm_tm->tm_mday >= 1) ?
			 bin2bcd((u32)alm_tm->tm_mday) :
			 AT8563_ALM_BIT_DISABLE;

	buf[3] = (alm_tm->tm_wday < 7 && alm_tm->tm_wday >= 0) ?
			 bin2bcd((u32)alm_tm->tm_wday) :
			 AT8563_ALM_BIT_DISABLE;

	ret = i2c_smbus_write_i2c_block_data(data, AT8563_ALM_MIN, 4, buf);
	if (ret < 0)
		return ret;

	return AT8563_rtc_alarm_irq_enable(data, alm->enabled);
}

static void at8563_work(void *param)
{
	struct AT8563 *at8563 = (struct AT8563 *)param;

	struct mutex *lock = &at8563->ops_lock;
	int data, ret;
	mutex_lock(lock);

	rtc_dbg("%s\n", __func__);
	/* Clear the alarm flag */

	data = i2c_smbus_read_byte_data(at8563, AT8563_CTL2);
	if (data < 0) {
		rtc_dbg("%s: error reading i2c data %d\n", __func__, data);
		goto out;
	}

	data &= ~AT8563_CTL2_AF;

	ret = i2c_smbus_write_byte_data(at8563, AT8563_CTL2, data);
	if (ret < 0) {
		rtc_dbg("%s: error writing i2c data %d\n", __func__, ret);
	}

out:
	mutex_unlock(lock);
	return;
}

static void at8563_irq(uint32_t param)
{
	struct AT8563 *at8563 = (struct AT8563 *)param;

	if (work_available(&at8563->work)) {
		work_queue(HPWORK, &at8563->work, at8563_work, (void *)at8563, 0);
	}
}

static int rtc_at8563_open(struct file *filep)
{
	return 0;
}

static int rtc_at8563_close(struct file *filep)
{
	return 0;
}
static ssize_t rtc_at8563_read(struct file *filep, char *buf, size_t size)
{
	return 0;
}

static ssize_t rtc_at8563_write(struct file *filep, const char *buf, size_t size)
{
	return 0;
}

static int rtc_at8563_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int err;
	struct inode *inode = filep->f_inode;
	struct AT8563 *at8563 = inode->i_private;
	struct rtc_wkalrm alarm;
	struct rtc_time tm;
	void __user *uarg = (void __user *)arg;

	mutex_lock_interruptible(&at8563->ops_lock);

	switch (cmd) {
	case RTC_ALM_READ:
		err = AT8563_rtc_read_alarm(at8563, &alarm);
		if (err < 0)
			goto out;
		memcpy(uarg, &alarm.time, sizeof(tm));
		mutex_unlock(&at8563->ops_lock);
		break;
	case RTC_ALM_SET:
		memcpy(&alarm.time, uarg, sizeof(tm));
		alarm.enabled = 0;
		alarm.pending = 0;
		alarm.time.tm_wday = -1;
		alarm.time.tm_yday = -1;
		alarm.time.tm_isdst = -1;
		/* RTC_ALM_SET alarms may be up to 24 hours in the future.
		 * Rather than expecting every RTC to implement "don't care"
		 * for day/month/year fields, just force the alarm to have
		 * the right values for those fields.
		 *
		 * RTC_WKALM_SET should be used instead.  Not only does it
		 * eliminate the need for a separate RTC_AIE_ON call, it
		 * doesn't have the "alarm 23:59:59 in the future" race.
		 *
		 * NOTE:  some legacy code may have used invalid fields as
		 * wildcards, exposing hardware "periodic alarm" capabilities.
		 * Not supported here.
		 */
		{
			time64_t now, then;

			err = AT8563_rtc_read_time(at8563, &tm);
			if (err < 0)
				goto out;
			now = rtc_tm_to_time64(&tm);

			alarm.time.tm_mday = tm.tm_mday;
			alarm.time.tm_mon = tm.tm_mon;
			alarm.time.tm_year = tm.tm_year;
			err = rtc_valid_tm(&alarm.time);
			if (err < 0)
				goto out;
			then = rtc_tm_to_time64(&alarm.time);

			/* alarm may need to wrap into tomorrow */
			if (then < now) {
				rtc_time64_to_tm(now + 24 * 60 * 60, &tm);
				alarm.time.tm_mday = tm.tm_mday;
				alarm.time.tm_mon = tm.tm_mon;
				alarm.time.tm_year = tm.tm_year;
			}
		}

		time64_t now_time, scheduled;
		err = rtc_valid_tm(&alarm.time);
		if (err)
			goto out;
		scheduled = rtc_tm_to_time64(&alarm.time);

		/* Make sure we're not setting alarms in the past */
		err = AT8563_rtc_read_time(at8563, &tm);
		if (err)
			goto out;
		now_time = rtc_tm_to_time64(&tm);
		if (scheduled <= now_time) {
			printf("alarm time must bigger than now\n");
			goto out;
		}
		alarm.enabled = 1;
		AT8563_rtc_set_alarm(at8563, &alarm);
		mutex_unlock(&at8563->ops_lock);
		break;
	case RTC_RD_TIME:
		AT8563_rtc_read_time(at8563, &tm);
		err = rtc_valid_tm(&tm);
		if (err < 0)
			dev_dbg(&rtc->dev, "read_time: rtc_time isn't valid\n");
		memcpy(uarg, &tm, sizeof(tm));
		mutex_unlock(&at8563->ops_lock);
		break;
	case RTC_SET_TIME:
		memcpy(&tm, uarg, sizeof(tm));
		err = rtc_valid_tm(&tm);
		if (err != 0) {
			dev_dbg(&rtc->dev, "read_time: rtc_time isn't valid\n");
			goto out;
		}
		int days;
		unsigned int secs;
		days = div_s64_rem(rtc_tm_to_time64(&tm), 86400, &secs);
		/* day of the week, 1970-01-01 was a Thursday */
		tm.tm_wday = (days + 4) % 7;

		AT8563_rtc_set_time(at8563, &tm);
		mutex_unlock(&at8563->ops_lock);
		break;
	case RTC_AIE_ON:
		AT8563_rtc_alarm_irq_enable(at8563, 1);
		mutex_unlock(&at8563->ops_lock);
		break;
	case RTC_AIE_OFF:
		AT8563_rtc_alarm_irq_enable(at8563, 0);
		mutex_unlock(&at8563->ops_lock);
		break;
#if 0 /* not support */
	case RTC_UIE_ON:
		break;
	case RTC_UIE_OFF:
		break;
	case RTC_IRQP_SET:
		break;
	case RTC_IRQP_READ:
		break;
	case RTC_PIE_ON:
		break;
	case RTC_PIE_OFF:
		break;
	case RTC_WKALM_SET:
		break;
	case RTC_WKALM_RD:
		break;
#endif
	default:
		break;
	}

	return 0;
out:
	mutex_unlock(&at8563->ops_lock);
	return err;
}

static const struct file_operations rtc_at8563_fops = {
	.open 	= 	rtc_at8563_open,
	.close 	= 	rtc_at8563_close,
	.read 	= 	rtc_at8563_read,
	.write 	= 	rtc_at8563_write,
	.poll 	= 	NULL,
	.ioctl 	= 	rtc_at8563_ioctl
};

static int AT8563_init_device(struct AT8563 *data)
{
	int ret;

	/* Clear stop flag if present */
	ret = i2c_smbus_write_byte_data(data, AT8563_CTL1, 0);
	if (ret < 0)
		return ret;

	ret = i2c_smbus_read_byte_data(data, AT8563_CTL2);
	if (ret < 0)
		return ret;

	/* Disable alarm and timer interrupts */
	ret &= ~AT8563_CTL2_AIE;
	ret &= ~AT8563_CTL2_TIE;

	/* Clear any pending alarm and timer flags */
	if (ret & AT8563_CTL2_AF)
		ret &= ~AT8563_CTL2_AF;

	if (ret & AT8563_CTL2_TF)
		ret &= ~AT8563_CTL2_TF;

	ret &= ~AT8563_CTL2_TI_TP;

	return i2c_smbus_write_byte_data(data, AT8563_CTL2, ret);
}

static int hc_rtc8563_init(void)
{
	int np = -1;
	int ret = -1;
	const char *path;
	struct AT8563 *at8563;

	np = fdt_get_node_offset_by_path("/hcrtos/at8563");
	if (np < 0) {
		rtc_dbg("can't find AT8563\n");
		return -ENODEV;
	}

	at8563 = kzalloc(sizeof(*at8563), GFP_KERNEL);
	if (at8563 == NULL)
		return -ENOMEM;

	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&at8563->addr))
		return -1;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &at8563->i2c_devpath))
		return -1;

	at8563->irq = PINPAD_INVALID;
	fdt_get_property_u_32_index(np, "irq-gpio", 0, (u32 *)&at8563->irq);
	if (at8563->irq != PINPAD_INVALID) {
		gpio_configure(at8563->irq, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
		ret = gpio_irq_request(at8563->irq, at8563_irq, (uint32_t)at8563);
		if (ret < 0) {
			printf("ERROR: gpio_irq_request() failed: %d\n", ret);
			return ret;
		}
	}

	if (fdt_get_property_string_index(np, "devpath", 0, &path))
		return 0;

	ret = AT8563_init_device(at8563);
	if (ret < 0) {
		dev_err(&client->dev, "could not init device, %d\n", ret);
		return ret;
	}

	mutex_init(&at8563->ops_lock);
	register_driver(path, &rtc_at8563_fops, 0666, at8563);

	/* check state of calendar information */
	ret = i2c_smbus_read_byte_data(at8563, AT8563_SEC);
	printf("lhg ret = %x\n", ret);
	if (ret < 0)
		return ret;

	return 0;
}

module_driver(hc_rtc8563_driver, hc_rtc8563_init, NULL, 1)
