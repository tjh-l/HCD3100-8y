/****************************************************************************
 * drivers/i2c/i2c_bitbang.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <stdio.h>

#include <nuttx/arch.h>
//#include <nuttx/spinlock.h>
#include <nuttx/semaphore.h>
#include <nuttx/kmalloc.h>
#include <nuttx/i2c/i2c_bitbang.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct i2c_bitbang_dev_s
{
	struct i2c_master_s i2c;
	struct i2c_bitbang_lower_dev_s *lower;
	uint32_t delay;
	int timeout;
};

/****************************************************************************
 * Start From Linux
 ****************************************************************************/
/* ----- global defines ----------------------------------------------- */
//#define DEBUG
#ifdef DEBUG
#define bit_dbg(format, args...)                                               \
	do {                                                                   \
		printf(format, ##args);                                        \
	} while (0)
#else
#define bit_dbg(format, args...)                                               \
	do {                                                                   \
	} while (0)
#endif /* DEBUG */

#define setsda(dev, val) 	dev->lower->ops->set_sda(dev->lower, val)
#define getsda(dev) 		dev->lower->ops->get_sda(dev->lower)
#define setscl(dev, val) 	dev->lower->ops->set_scl(dev->lower, val)
#define getscl(dev) 		dev->lower->ops->get_scl(dev->lower)

static inline void sdalo(struct i2c_bitbang_dev_s *priv)
{
	setsda(priv, 0);
	up_udelay((priv->delay + 1) / 2);
}

static inline void sdahi(struct i2c_bitbang_dev_s *priv)
{
	setsda(priv, 1);
	up_udelay((priv->delay + 1) / 2);
}

static inline void scllo(struct i2c_bitbang_dev_s *priv)
{
	setscl(priv, 0);
	up_udelay(priv->delay / 2);
}

/*
 * Raise scl line, and do checking for delays. This is necessary for slower
 * devices.
 */
static int sclhi(struct i2c_bitbang_dev_s *priv)
{
	setscl(priv, 1);

#if 0 /* scl only output clk */
	unsigned long start;
	/* Not all privters have scl sense line... */
//	if (!priv->getscl)
//		goto done;

	start = jiffies;
	while (!getscl(priv)) {
		/* This hw knows how to read the clock line, so we wait
		 * until it actually gets high.  This is safer as some
		 * chips may hold it low ("clock stretching") while they
		 * are processing data internally.
		 */
		if (time_after(jiffies, start + priv->timeout)) {
			/* Test one last time, as we may have been preempted
			 * between last check and timeout test.
			 */
			if (getscl(priv))
				break;
			return -ETIMEDOUT;
		}
	//	cpu_relax();
	}
#ifdef DEBUG
	if (jiffies != start && i2c_debug >= 3)
		pr_debug("i2c-algo-bit: needed %ld jiffies for SCL to go "
			 "high\n", jiffies - start);
#endif

#endif
done:
	up_udelay(priv->delay);
	return 0;
}

/* --- other auxiliary functions --------------------------------------	*/
static void i2c_start(struct i2c_bitbang_dev_s *priv)
{
	/* assert: scl, sda are high */
	setsda(priv, 0);
	up_udelay(priv->delay);
	scllo(priv);
}

static void i2c_repstart(struct i2c_bitbang_dev_s *priv)
{
	/* assert: scl is low */
	sdahi(priv);
	sclhi(priv);
	setsda(priv, 0);
	up_udelay(priv->delay);
	scllo(priv);
}

static void i2c_stop(struct i2c_bitbang_dev_s *priv)
{
	/* assert: scl is low */
	sdalo(priv);
	sclhi(priv);
	setsda(priv, 1);
	up_udelay(priv->delay);
}

/* send a byte without start cond., look for arbitration,
   check ackn. from slave */
/* returns:
 * 1 if the device acknowledged
 * 0 if the device did not ack
 * -ETIMEDOUT if an error occurred (while raising the scl line)
 */
static int i2c_outb(struct i2c_bitbang_dev_s *priv, unsigned char c)
{
	int i;
	int sb;
	int ack;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		setsda(priv, sb);
		up_udelay((priv->delay + 1) / 2);
		if (sclhi(priv) < 0) { /* timed out */
			bit_dbg("i2c_outb: 0x%02x, timeout at bit #%d\n",
				(int)c, i);
			return -ETIMEDOUT;
		}
		/* FIXME do arbitration here:
		 * if (sb && !getsda(priv)) -> ouch! Get out of here.
		 *
		 * Report a unique code, so higher level code can retry
		 * the whole (combined) message and *NOT* issue STOP.
		 */
		scllo(priv);
	}

	/* read ack: SDA should be pulled down by slave, or it may
	 * NAK (usually to report problems with the data we wrote).
	 */

	getsda(priv); /* ack: sda is pulled low -> success */

	up_udelay(priv->delay);
	if (sclhi(priv) < 0) { /* timeout */
		bit_dbg("i2c_outb: 0x%02x, timeout at ack\n", (int)c);
		return -ETIMEDOUT;
	}

	ack = !getsda(priv); /* ack: sda is pulled low -> success */
	if (ack)
		bit_dbg("%s:%d i2c_outb: 0x%02x %s\n", __func__, __LINE__,
		       (int)c, ack ? "A" : "NA");
	scllo(priv);

	return ack;
	/* assert: scl is low (sda undef) */
}

static int i2c_inb(struct i2c_bitbang_dev_s *priv)
{
	/* read byte via i2c port, without start/stop sequence	*/
	/* acknowledge is sent in i2c_read.			*/
	int i;
	unsigned char indata = 0;

	/* assert: scl is low */
	getsda(priv);
	up_udelay(priv->delay);
	for (i = 0; i < 8; i++) {
		if (sclhi(priv) < 0) { /* timeout */
			bit_dbg("i2c_inb: timeout at bit #%d\n", 7 - i);
			return -ETIMEDOUT;
		}
		indata *= 2;
		if (getsda(priv))
			indata |= 0x01;
		setscl(priv, 0);
		up_udelay(i == 7 ? priv->delay / 2 : priv->delay);
	}
	/* assert: scl is low */
	return indata;
}
/****************************************************************************
 * End From Linux
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int i2c_bitbang_transfer(FAR struct i2c_master_s *dev,
				FAR struct i2c_msg_s *msgs, int count);
static int i2c_bitbang_timeout(FAR struct i2c_master_s *dev,
			       unsigned long timeout);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static const struct i2c_ops_s g_i2c_ops =
{
	.transfer = i2c_bitbang_transfer,
	.timeout = i2c_bitbang_timeout
};

/****************************************************************************
 * Name: i2c_bitbang_timeout
 ****************************************************************************/
static int i2c_bitbang_timeout(FAR struct i2c_master_s *dev,
			       unsigned long timeout)
{
	return 0;
}

/* 
 * ----- Utility functions
 */

/* try_address tries to contact a chip for a number of
 * times before it gives up.
 * return values:
 * 1 chip answered
 * 0 chip did not answer
 * -x transmission error
 */
static int try_address(struct i2c_bitbang_dev_s *priv, unsigned char addr,
		       int retries)
{
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = i2c_outb(priv, addr);
		if (ret == 1 || i == retries)
			break;
		bit_dbg("emitting stop condition\n");
		i2c_stop(priv);
		up_udelay(priv->delay);
		//yield();
		bit_dbg("emitting start condition\n");
		i2c_start(priv);
	}
	if (i && ret)
		bit_dbg("Used %d tries to %s client at "
			"0x%02x: %s\n",
			i + 1, addr & 1 ? "read from" : "write to", addr >> 1,
			ret == 1 ? "success" : "failed, timeout?");
		return ret;
}

static int sendbytes(struct i2c_bitbang_dev_s *priv, struct i2c_msg_s *msg)
{
	const unsigned char *temp = msg->buffer;
	int count = msg->length;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	int retval;
	int wrcount = 0;

	while (count > 0) {
		retval = i2c_outb(priv, *temp);

		/* OK/ACK; or ignored NAK */
		if ((retval > 0) || (nak_ok && (retval == 0))) {
			count--;
			temp++;
			wrcount++;

			/* A slave NAKing the master means the slave didn't like
		 * something about the data it saw.  For example, maybe
		 * the SMBus PEC was wrong.
		 */
		} else if (retval == 0) {
			bit_dbg("sendbytes: NAK bailout.\n");
			/* ignore the write ack */
		//	return -EIO;
			count--;
			temp++;
			wrcount++;
			/* Timeout; or (someday) lost arbitration
		 *
		 * FIXME Lost ARB implies retrying the transaction from
		 * the first message, after the "winning" master issues
		 * its STOP.  As a rule, upper layer code has no reason
		 * to know or care about this ... it is *NOT* an error.
		 */
		} else {
			bit_dbg("sendbytes: error %d\n", retval);
			return retval;
		}
	}

	/* To End This transfer */
	scllo(priv);
	sdalo(priv);
	up_udelay((priv->delay + 1) / 2);
	sclhi(priv);
	up_udelay((priv->delay + 1) / 2);
	sdahi(priv);

	return wrcount;
}

static int acknak(struct i2c_bitbang_dev_s *priv, int is_ack)
{
	/* assert: sda is high */
	if (is_ack) /* send ack */
		setsda(priv, 0);
	up_udelay((priv->delay + 1) / 2);
	if (sclhi(priv) < 0) { /* timeout */
		bit_dbg("readbytes: ack/nak timeout\n");
		return -ETIMEDOUT;
	}
	scllo(priv);
	return 0;
}

static int readbytes(struct i2c_bitbang_dev_s *priv, struct i2c_msg_s *msg)
{
	int inval;
	int rdcount = 0; /* counts bytes read */
	unsigned char *temp = msg->buffer;
	int count = msg->length;
	const unsigned flags = msg->flags;

	while (count > 0) {
		inval = i2c_inb(priv);
		if (inval >= 0) {
			*temp = inval;
			rdcount++;
		} else { /* read timed out */
			break;
		}

		temp++;
		count--;

		/* Some SMBus transactions require that we receive the
		   transaction length as the first read byte. */
		if (rdcount == 1 && (flags & I2C_M_RECV_LEN)) {
			if (inval <= 0 || inval > I2C_SMBUS_BLOCK_MAX) {
				if (!(flags & I2C_M_NO_RD_ACK))
					acknak(priv, 0);
				bit_dbg("readbytes: invalid "
				       "block length (%d)\n",
				       inval);
				return -EPROTO;
			}
			/* The original count value accounts for the extra
			   bytes, that is, either 1 for a regular transaction,
			   or 2 for a PEC transaction. */
			count += inval;
			msg->length += inval;
		}

		bit_dbg("readbytes: 0x%02x %s\n", inval,
			(flags & I2C_M_NO_RD_ACK) ? "(no ack/nak)" :
						    (count ? "A" : "NA"));

		if (!(flags & I2C_M_NO_RD_ACK)) {
			inval = acknak(priv, count);
			if (inval < 0)
				return inval;
		}
	}
	return rdcount;
}

/* doAddress initiates the transfer by generating the start condition (in
 * try_address) and transmits the address in the necessary format to handle
 * reads, writes as well as 10bit-addresses.
 * returns:
 *  0 everything went okay, the chip ack'ed, or IGNORE_NAK flag was set
 * -x an error occurred (like: -ENXIO if the device did not answer, or
 *	-ETIMEDOUT, for example if the lines are stuck...)
 */
static int bit_doAddress(struct i2c_bitbang_dev_s *priv, struct i2c_msg_s *msg)
{
	unsigned short flags = msg->flags;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;

	unsigned char addr;
	int ret, retries;

	/* retries = nak_ok ? 0 : i2c_priv->retries; */
	retries = nak_ok ? 0 : 3;

	if (flags & I2C_M_TEN) {
		/* a ten bit address */
		addr = 0xf0 | ((msg->addr >> 7) & 0x06);
		bit_dbg("addr0: %d\n", addr);
		/* try extended address code...*/
		ret = try_address(priv, addr, retries);
		if ((ret != 1) && !nak_ok) {
			bit_dbg("died at extended address code\n");
			return -ENXIO;
		}
		/* the remaining 8 bit address */
		ret = i2c_outb(priv, msg->addr & 0xff);
		if ((ret != 1) && !nak_ok) {
			/* the chip did not ack / xmission error occurred */
			bit_dbg("died at 2nd address code\n");
			return -ENXIO;
		}
		if (flags & I2C_M_RD) {
			bit_dbg("emitting repeated start condition\n");
			i2c_repstart(priv);
			/* okay, now switch into reading mode */
			addr |= 0x01;
			ret = try_address(priv, addr, retries);
			if ((ret != 1) && !nak_ok) {
				bit_dbg("died at repeated address code\n");
				return -EIO;
			}
		}
	} else { /* normal 7bit address	*/
		addr = msg->addr << 1;
		if (flags & I2C_M_RD)
			addr |= 1;
		if (flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;
		ret = try_address(priv, addr, retries);
		if ((ret != 1) && !nak_ok)
			return -ENXIO;
	}

	return 0;
}

/****************************************************************************
 * Name: i2c_bitbang_transfer
 ****************************************************************************/
static int i2c_bitbang_transfer(FAR struct i2c_master_s *dev,
				FAR struct i2c_msg_s *msgs, int count)
{
	FAR struct i2c_bitbang_dev_s *priv =
		(FAR struct i2c_bitbang_dev_s *)dev;
	int ret = OK;
	int i;

	struct i2c_msg_s *pmsg;
	unsigned short nak_ok;
	/* Lock to enforce timings */
	taskENTER_CRITICAL();

	i2c_start(priv);
	for (i = 0; i < count; i++) {
		pmsg = &msgs[i];
		nak_ok = pmsg->flags & I2C_M_IGNORE_NAK;
		if (!(pmsg->flags & I2C_M_NOSTART)) {
			if (i) {
				bit_dbg("emitting repeated start condition\n");
				i2c_repstart(priv);
			}
			ret = bit_doAddress(priv, pmsg);
			if ((ret != 0) && !nak_ok) {
				bit_dbg("NAK from device addr 0x%02x msg #%d\n",
					msgs[i].addr, i);
				goto bailout;
			}
		}
		if (pmsg->flags & I2C_M_RD) {
			/* read bytes into buffer*/
			ret = readbytes(priv, pmsg);
			if (ret >= 1)
				bit_dbg("read %d byte%s\n", ret,
					ret == 1 ? "" : "s");
			if (ret < pmsg->length) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		} else {
			/* write bytes from buffer */
			ret = sendbytes(priv, pmsg);
			if (ret >= 1)
				bit_dbg("wrote %d byte%s\n", ret,
					ret == 1 ? "" : "s");
			if (ret < pmsg->length) {
				if (ret >= 0)
					ret = -EIO;
				goto bailout;
			}
		}
	}
	ret = i;

bailout:
	bit_dbg("emitting stop condition\n");
	i2c_stop(priv);

	taskEXIT_CRITICAL();
	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2c_bitbang_initialize
 *
 * Description:
 *   Initialize a bitbang I2C device instance
 *
 * Input Parameters:
 *   lower  - Lower half of driver
 *
 * Returned Value:
 *   Pointer to a the I2C instance
 *
 ****************************************************************************/
FAR struct i2c_master_s *
i2c_bitbang_initialize(FAR struct i2c_bitbang_lower_dev_s *lower)
{
	FAR struct i2c_bitbang_dev_s *dev;
	struct hc_i2c_bitbang_dev_s *priv =
		(struct hc_i2c_bitbang_dev_s *)lower->priv;

	DEBUGASSERT(lower && lower->ops);

	dev = (FAR struct i2c_bitbang_dev_s *)kmm_zalloc(sizeof(*dev));

	if (!dev) {
		return NULL;
	}

	dev->i2c.ops = &g_i2c_ops;
	dev->lower = lower;
	dev->lower->ops->initialize(dev->lower);
	dev->delay = priv->delay;
	dev->timeout = priv->timeout;

	return &dev->i2c;
}

