#include <errno.h>
#include <unistd.h>
#include <asm-generic/page.h>
#include <asm-generic/param.h>
#include <asm-generic/div64.h>
#include <linux/bits.h>
#include <linux/sizes.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/minmax.h>
#include <linux/mtd/spi-nor.h>
#include <linux/log2.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/math64.h>
#include <linux/mtd/nand.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>
#include <linux/spi/spi-mem.h>

#include "mt29f_spinand.h"

#define BUFSIZE (10 * 64 * 2048)
#define CACHE_BUF 2112
/*
 * OOB area specification layout:  Total 32 available free bytes.
 */

static void spinand_reset(struct spi_device *spi_nand);
static int spinand_lock_block(struct spi_device *spi_nand, u8 lock);

static inline struct spinand_state *mtd_to_state(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct spinand_info *info = (struct spinand_info *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)info->priv;

	return state;
}

static struct nand_ecclayout spinand_oob_64 = {
	.eccbytes = 24,
	.eccpos = {
		1, 2, 3, 4, 5, 6,
		17, 18, 19, 20, 21, 22,
		33, 34, 35, 36, 37, 38,
		49, 50, 51, 52, 53, 54, },
	.oobavail = 32,
	.oobfree = {
		{.offset = 8,
			.length = 8},
		{.offset = 24,
			.length = 8},
		{.offset = 40,
			.length = 8},
		{.offset = 56,
			.length = 8},
	}
};

/*
 * spinand_cmd - to process a command to send to the SPI Nand
 * Description:
 *    Set up the command buffer to send to the SPI controller.
 *    The command buffer has to initialized to 0.
 */

static int __spinand_cmd(struct spi_device *spi, struct spinand_cmd *cmd)
{
	struct spi_message message;
	struct spi_transfer x[4];
	u8 dummy = 0xff;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));

	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	spi_message_add_tail(&x[0], &message);

	if (cmd->n_addr) {
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
	}

	if (cmd->n_dummy) {
		x[2].len = cmd->n_dummy;
		x[2].tx_buf = &dummy;
		spi_message_add_tail(&x[2], &message);
	}

	if (cmd->n_tx) {
		x[3].len = cmd->n_tx;
		x[3].tx_buf = cmd->tx_buf;

		if (cmd->cmd == CMD_PROG_PAGE_CLRCACHE_X4)
			x[3].tx_nbits = SPI_NBITS_QUAD;
		else
			x[3].tx_nbits = SPI_NBITS_SINGLE;

		spi_message_add_tail(&x[3], &message);
	}

	if (cmd->n_rx) {
		x[3].len = cmd->n_rx;
		x[3].rx_buf = cmd->rx_buf;

		if (cmd->cmd == CMD_READ_RDM_X4)
			x[3].rx_nbits = SPI_NBITS_QUAD;
		else if (cmd->cmd == CMD_READ_RDM_X2)
			x[3].rx_nbits = SPI_NBITS_DUAL;
		else
			x[3].rx_nbits = SPI_NBITS_SINGLE;

		spi_message_add_tail(&x[3], &message);
	}

	return spi_sync(spi, &message);
}

static int __spinand_cmd2(struct spi_device *spi, struct spinand_cmd *cmd)
{
	struct spi_message message;
	struct spi_transfer x[1];
	u8 dummy = 0xff;
	int ret = 0;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));

	x[0].cmd = cmd->cmd;

	if (cmd->n_addr) {
		x[0].addr = cmd->addr;
		x[0].n_addr = cmd->n_addr;
	}

	if (cmd->n_dummy)
		x[0].dummy_data = 1;

	if (cmd->n_tx) {
		if (cmd->cmd == CMD_PROG_PAGE_CLRCACHE_X4)
			x[0].tx_nbits = SPI_NBITS_QUAD;
		else
			x[0].tx_nbits = SPI_NBITS_SINGLE;

		x[0].tx_len = cmd->n_tx;
		x[0].tx_buf = cmd->tx_buf;
	}

	if (cmd->n_rx) {
		if (cmd->cmd == CMD_READ_RDM_X4)
			x[0].rx_nbits = SPI_NBITS_QUAD;
		else if (cmd->cmd == CMD_READ_RDM_X2)
			x[0].rx_nbits = SPI_NBITS_DUAL;
		else
			x[0].rx_nbits = SPI_NBITS_SINGLE;

		x[0].rx_len = cmd->n_rx;
		x[0].rx_buf = cmd->rx_buf;
	}

	x[0].hw_transfers = true;

	spi_message_add_tail(&x[0], &message);

	ret = spi_sync(spi, &message);

	return ret;
}

static int spinand_cmd(struct spi_device *spi, struct spinand_cmd *cmd)
{
	if ((cmd->n_tx == 0) && (cmd->n_rx == 0) && (cmd->n_addr != 0))
		return __spinand_cmd(spi, cmd);
	else
		return __spinand_cmd2(spi, cmd);
}

/*
 * spinand_read_id- Read SPI Nand ID
 * Description:
 *    Read ID: read two ID bytes from the SPI Nand device
 */
static int spinand_read_id(struct spi_device *spi_nand, u8 *id)
{
	int retval;
	u8 nand_id[4];
	struct spinand_cmd cmd = {0};

	cmd.cmd = CMD_READ_ID;
	cmd.n_rx = 4;
	cmd.rx_buf = &nand_id[0];
	cmd.n_dummy = 1;

	retval = spinand_cmd(spi_nand, &cmd);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "error %d reading id\n", retval);
		return retval;
	}
	id[0] = nand_id[0];
	id[1] = nand_id[1];
	dev_info("%s:%d nand_id[0] 0x%x, nand_id[1] 0x%x\n", __func__, __LINE__, nand_id[0], nand_id[1]);

	return retval;
}

/*
 * spinand_read_status- send command 0xf to the SPI Nand status register
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the
 *    busy status.
 *    This function is to allow reading the status of the command: read,
 *    write, and erase.
 *    Once the status turns to be ready, the other status bits also are
 *    valid status bits.
 */
static int spinand_read_status(struct spi_device *spi_nand, u8 *status)
{
	struct spinand_cmd cmd = {0};
	int ret;

	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "err: %d read status register\n", ret);
	return ret;
}

static int spinand_read_status_a0h(struct spi_device *spi_nand, u8 *status)
{
	struct spinand_cmd cmd = {0};
	int ret;

	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_A0H_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "err: %d read status register\n", ret);

	return ret;
}

static int spinand_read_status_f0h(struct spi_device *spi_nand, u8 *status)
{
	struct spinand_cmd cmd = {0};
	int ret;

	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_F0H_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "err: %d read status register\n", ret);

	return ret;
}

#define MAX_WAIT_JIFFIES  (CONFIG_MTD_SPINAND_TIMEOUT * HZ)
static int wait_till_ready(struct spi_device *spi_nand)
{
	unsigned long deadline;
	int retval;
	u8 stat = 0;

	deadline = jiffies + MAX_WAIT_JIFFIES;
	do {
		retval = spinand_read_status(spi_nand, &stat);
		if (retval < 0)
			return -1;
		else if (!(stat & 0x1))
			break;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	if ((stat & 0x1) == 0)
		return 0;

	return -1;
}

/**
 * spinand_get_otp- send command 0xf to read the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_get_otp(struct spi_device *spi_nand, u8 *otp)
{
	struct spinand_cmd cmd = {0};
	int retval;

	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_rx = 1;
	cmd.rx_buf = otp;

	retval = spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		dev_err(&spi_nand->dev, "error %d get otp\n", retval);
	return retval;
}

/**
 * spinand_set_otp- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_set_otp(struct spi_device *spi_nand, u8 *otp)
{
	int retval;
	struct spinand_cmd cmd = {0};

	cmd.cmd = CMD_WRITE_REG,
	cmd.n_addr = 1,
	cmd.addr[0] = REG_OTP,
	cmd.n_tx = 1,
	cmd.tx_buf = otp,

	retval = spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		dev_err(&spi_nand->dev, "error %d set otp\n", retval);

	return retval;
}

/**
 * spinand_enable_ecc- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_enable_ecc(struct spi_device *spi_nand)
{
	int retval;
	u8 otp = 0;

	retval = spinand_get_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;

	if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK)
		return 0;
	otp |= OTP_ECC_MASK;
	retval = spinand_set_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;
	return spinand_get_otp(spi_nand, &otp);
}

static int spinand_disable_ecc(struct spi_device *spi_nand)
{
	int retval;
	u8 otp = 0;

	retval = spinand_get_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;

	if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK) {
		otp &= ~OTP_ECC_MASK;
		retval = spinand_set_otp(spi_nand, &otp);
		if (retval < 0)
			return retval;
		return spinand_get_otp(spi_nand, &otp);
	}
	return 0;
}

static int spinand_enable_quad_mode(struct spi_device *spi_nand)
{
	int retval;
	u8 otp = 0;

	retval = spinand_get_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;

	if ((otp & QUAD_MODE_MASK) == QUAD_MODE_MASK)
		return 0;

	otp |= QUAD_MODE_MASK;
	retval = spinand_set_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;
	return spinand_get_otp(spi_nand, &otp);
}

static int spinand_disable_quad_mode(struct spi_device *spi_nand)
{
	int retval;
	u8 otp = 0;

	retval = spinand_get_otp(spi_nand, &otp);
	if (retval < 0)
		return retval;

	if ((otp & QUAD_MODE_MASK) == QUAD_MODE_MASK) {
		otp &= ~QUAD_MODE_MASK;
		retval = spinand_set_otp(spi_nand, &otp);
		if (retval < 0)
			return retval;
		return spinand_get_otp(spi_nand, &otp);
	}
	return 0;
}

/**
 * spinand_write_enable- send command 0x06 to enable write or erase the
 * Nand cells
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically
 *   cleared (status register bit 2)
 *   Set the bit 2 of the status register has the same effect
 */
static int spinand_write_enable(struct spi_device *spi_nand)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = CMD_WR_ENABLE;
	return spinand_cmd(spi_nand, &cmd);
}

static int spinand_read_page_to_cache(struct spi_device *spi_nand, u32 page_id)
{
	struct spinand_cmd cmd = {0};
	u32 row;

	row = page_id;
	cmd.cmd = CMD_READ;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row & 0xff0000) >> 16);
	cmd.addr[1] = (u8)((row & 0xff00) >> 8);
	cmd.addr[2] = (u8)(row & 0x00ff);

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_read_from_cache- send command 0x03 to read out the data from the
 * cache register(2112 bytes max)
 * Description:
 *   The read can specify 1 to 2112 bytes of data read at the corresponding
 *   locations.
 *   No tRd delay.
 */
static int spinand_read_from_cache(struct spi_device *spi_nand, u16 page_id,
				   u16 byte_id, u16 len, u8 *rbuf)
{
	int retval;
	struct spinand_cmd cmd = {0};

	u16 column;
	uint16_t mode = spi_nand->mode;

	column = byte_id;

	if ((mode & SPI_RX_QUAD) == SPI_RX_QUAD )
		cmd.cmd = CMD_READ_RDM_X4;
	else if ((mode & SPI_RX_DUAL) == SPI_RX_DUAL)
		cmd.cmd = CMD_READ_RDM_X2;
	else
		cmd.cmd = CMD_READ_RDM;

	cmd.n_addr = 2;
	cmd.addr[0] = (u8)((column & 0xff00) >> 8);
	cmd.addr[0] |= (u8)(((page_id >> 6) & 0x1) << 4);
	cmd.addr[1] = (u8)(column & 0x00ff);
	cmd.n_dummy = 1;
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;

	/* enable spi nand quad mode, if mode == SPI_RX_QUAD */
	if ((mode & SPI_RX_QUAD) == SPI_RX_QUAD) {
		retval = spinand_enable_quad_mode(spi_nand);
		if (retval < 0)
			return retval;
	}

	retval = spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		return retval;

	/* disable spi nand quad mode, if mode == SPI_RX_QUAD */
	if ((mode & SPI_RX_QUAD) == SPI_RX_QUAD) {
		retval = spinand_disable_quad_mode(spi_nand);
		if (retval < 0)
			return retval;
	}

	return retval;
}

/*
 * spinand_read_page-to read a page with:
 * @page_id: the physical page number
 * @offset:  the location from 0 to 2111
 * @len:     number of bytes to read
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The read includes two commands to the Nand: 0x13 and 0x03 commands
 *   Poll to read status to wait for tRD time.
 */
static int spinand_read_page(struct spi_device *spi_nand, u32 page_id,
			     u16 offset, u16 len, u8 *rbuf)
{
	int ret;
	u8 status = 0;

	ret = spinand_read_page_to_cache(spi_nand, page_id);
	if (ret < 0)
		return ret;

	if (wait_till_ready(spi_nand))
		dev_err(&spi_nand->dev, "WAIT timedout!!!\n");

	while (1) {
		ret = spinand_read_status(spi_nand, &status);
		if (ret < 0) {
			dev_err(&spi_nand->dev,
				"err %d read status register\n", ret);
			return ret;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			if ((status & STATUS_ECC_MASK) == STATUS_ECC_ERROR) {
				dev_err(&spi_nand->dev, "r c0h ecc error: 0x%02x, page=%d\n", status, page_id);
				spinand_read_status_f0h(spi_nand, &status);
				dev_err(&spi_nand->dev, "r f0h ecc error: 0x%02x, page=%d\n", status, page_id);
				return -EBADMSG;
			}
			break;
		}
	}

	ret = spinand_read_from_cache(spi_nand, page_id, offset, len, rbuf);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "read from cache failed!!\n");
		return ret;
	}

	return ret;
}

/*
 * spinand_program_data_to_cache--to write a page to cache with:
 * @byte_id: the location to write to the cache
 * @len:     number of bytes to write
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The write command used here is 0x84--indicating that the cache is
 *   not cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
static int spinand_program_data_to_cache(struct spi_device *spi_nand,
					 u16 page_id, u16 byte_id,
					 u16 len, u8 *wbuf)
{
	int retval;
	struct spinand_cmd cmd = {0};
	u16 column;
	uint16_t mode = spi_nand->mode;

	if ((mode & SPI_TX_QUAD) == SPI_TX_QUAD)
		cmd.cmd = CMD_PROG_PAGE_CLRCACHE_X4;
	else
		cmd.cmd = CMD_PROG_PAGE_CLRCACHE;

	column = byte_id;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)((column & 0xff00) >> 8);
	cmd.addr[0] |= (u8)(((page_id >> 6) & 0x1) << 4);
	cmd.addr[1] = (u8)(column & 0x00ff);
	cmd.n_tx = len;
	cmd.tx_buf = wbuf;

	/* enable spi nand quad mode, if mode == SPI_TX_QUAD */
	if ((mode & SPI_TX_QUAD) == SPI_TX_QUAD) {
		retval = spinand_enable_quad_mode(spi_nand);
		if (retval < 0)
			return retval;
	}

	retval = spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		return retval;

	/* disable spi nand quad mode, if mode == SPI_TX_QUAD */
	if ((mode & SPI_TX_QUAD) == SPI_TX_QUAD) {
		retval = spinand_disable_quad_mode(spi_nand);
		if (retval < 0)
			return retval;
	}

	return retval;

}

/**
 * spinand_program_execute--to write a page from cache to the Nand array with
 * @page_id: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to
 *   the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
static int spinand_program_execute(struct spi_device *spi_nand, u32 page_id)
{
	struct spinand_cmd cmd = {0};
	u32 row;

	row = page_id;
	cmd.cmd = CMD_PROG_PAGE_EXC;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row & 0xff0000) >> 16);
	cmd.addr[1] = (u8)((row & 0xff00) >> 8);
	cmd.addr[2] = (u8)(row & 0x00ff);

	return spinand_cmd(spi_nand, &cmd);
}

/**
 * spinand_program_page--to write a page with:
 * @page_id: the physical page location to write the page.
 * @offset:  the location from the cache starting from 0 to 2111
 * @len:     the number of bytes to write
 * @wbuf:    the buffer to hold the number of bytes
 *
 * Description:
 *   The commands used here are 0x06, 0x84, and 0x10--indicating that
 *   the write enable is first sent, the write cache command, and the
 *   write execute command.
 *   Poll to wait for the tPROG time to finish the transaction.
 */
static int spinand_program_page(struct spi_device *spi_nand,
				u32 page_id, u16 offset, u16 len, u8 *buf)
{
	int retval;
	u8 status = 0;
	u8 *wbuf;
	u8 freed = 1;

	wbuf = buf;
	retval = spinand_write_enable(spi_nand);
	if (retval < 0) {
		if (!freed)
			devm_kfree(&spi_nand->dev, wbuf);
		dev_err(&spi_nand->dev, "write enable failed!!\n");
		return retval;
	}
	if (wait_till_ready(spi_nand))
		dev_err(&spi_nand->dev, "wait timedout!!!\n");

	retval = spinand_program_data_to_cache(spi_nand, page_id,
					       offset, len, wbuf);
	if (retval < 0) {
		if (!freed)
			devm_kfree(&spi_nand->dev, wbuf);
		return retval;
	}
	retval = spinand_program_execute(spi_nand, page_id);
	if (retval < 0) {
		if (!freed)
			devm_kfree(&spi_nand->dev, wbuf);
		return retval;
	}
	while (1) {
		retval = spinand_read_status(spi_nand, &status);
		if (retval < 0) {
			if (!freed)
				devm_kfree(&spi_nand->dev, wbuf);
			dev_err(&spi_nand->dev,
				"error %d reading status register\n", retval);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			if ((status & STATUS_P_FAIL_MASK) == STATUS_P_FAIL) {
				if (!freed)
					devm_kfree(&spi_nand->dev, wbuf);
				dev_err(&spi_nand->dev,
					"program error, page %d\n", page_id);
				return -1;
			}
			if ((status & STATUS_ECC_MASK) == STATUS_ECC_ERROR) {
				dev_warn(&spi_nand->dev, "p c0h ecc error: 0x%02x, page=%d\n", status, page_id);
				spinand_read_status_f0h(spi_nand, &status);
				dev_warn(&spi_nand->dev, "p f0h ecc error: 0x%02x, page=%d\n", status, page_id);
				return -EBADMSG;
			}
			break;
		}
	}
	if (!freed)
		devm_kfree(&spi_nand->dev, wbuf);

	return 0;
}

/**
 * spinand_erase_block_erase--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase
 *   one block--64 pages
 *   Need to wait for tERS.
 */
static int spinand_erase_block_erase(struct spi_device *spi_nand, u32 block_id)
{
	struct spinand_cmd cmd = {0};
	u32 row;

	row = block_id;
	cmd.cmd = CMD_ERASE_BLK;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row & 0xff0000) >> 16);
	cmd.addr[1] = (u8)((row & 0xff00) >> 8);
	cmd.addr[2] = (u8)(row & 0x00ff);

	return spinand_cmd(spi_nand, &cmd);
}

/**
 * spinand_erase_block--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The commands used here are 0x06 and 0xd8--indicating an erase
 *   command to erase one block--64 pages
 *   It will first to enable the write enable bit (0x06 command),
 *   and then send the 0xd8 erase command
 *   Poll to wait for the tERS time to complete the tranaction.
 */
static int spinand_erase_block(struct spi_device *spi_nand, u32 block_id)
{
	int retval;
	u8 status = 0;

	retval = spinand_write_enable(spi_nand);
	if (wait_till_ready(spi_nand))
		dev_err(&spi_nand->dev, "wait timedout!!!\n");

	retval = spinand_erase_block_erase(spi_nand, block_id);
	while (1) {
		retval = spinand_read_status(spi_nand, &status);
		if (retval < 0) {
			dev_err(&spi_nand->dev,
				"error %d reading status register\n", retval);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			if ((status & STATUS_E_FAIL_MASK) == STATUS_E_FAIL) {
				dev_err(&spi_nand->dev,
					"erase error, block %d\n", block_id);
				dev_err(&spi_nand->dev, "r c0h ecc error: 0x%02x, page=%d\n", status, block_id);
				spinand_read_status_f0h(spi_nand, &status);
				dev_err(&spi_nand->dev, "r f0h ecc error: 0x%02x, page=%d\n", status, block_id);
				return -1;
			}
			break;
		}
	}
	return 0;
}

static int spinand_write_page_hwecc(struct mtd_info *mtd,
				    struct nand_chip *chip,
				    const u8 *buf, int oob_required,
				    int page)
{
	const u8 *p = buf;
	int eccsize = chip->ecc.size;
	int eccsteps = chip->ecc.steps;

	chip->write_buf(mtd, p, eccsize * eccsteps);
	return 0;
}

static int spinand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				   u8 *buf, int oob_required, int page)
{
	int retval;
	u8 status;
	u8 *p = buf;
	int eccsize = chip->ecc.size;
	int eccsteps = chip->ecc.steps;
	struct spinand_info *info = (struct spinand_info *)chip->priv;

	chip->read_buf(mtd, p, eccsize * eccsteps);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	while (1) {
		retval = spinand_read_status(info->spi, &status);
		if (retval < 0) {
			dev_err(&mtd->dev,
				"error %d reading status register\n", retval);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			if ((status & STATUS_ECC_MASK) == STATUS_ECC_ERROR) {
				pr_info("spinand: ECC error 0x%02x\n", status);
				mtd->ecc_stats.failed++;
			} else if ((status & STATUS_ECC_MASK) ==
					STATUS_ECC_1BIT_CORRECTED)
				mtd->ecc_stats.corrected++;
			break;
		}
	}
	return 0;
}

static void spinand_select_chip(struct mtd_info *mtd, int dev)
{
}

static u8 spinand_read_byte(struct mtd_info *mtd)
{
	struct spinand_state *state = mtd_to_state(mtd);
	u8 data;

	data = state->buf[state->buf_ptr];
	state->buf_ptr++;
	return data;
}

static int spinand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct spinand_info *info = (struct spinand_info *)chip->priv;

	unsigned long timeo = jiffies;
	int retval, state = chip->state;
	u8 status;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	while (time_before(jiffies, timeo)) {
		retval = spinand_read_status(info->spi, &status);
		if (retval < 0) {
			dev_err(&mtd->dev,
				"error %d reading status register\n", retval);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY)
			return 0;

		cond_resched();
	}
	return 0;
}

static void spinand_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	struct spinand_state *state = mtd_to_state(mtd);

	memcpy(state->buf + state->buf_ptr, buf, len);
	state->buf_ptr += len;
}

static void spinand_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
	struct spinand_state *state = mtd_to_state(mtd);

	memcpy(buf, state->buf + state->buf_ptr, len);
	state->buf_ptr += len;
}

/*
 * spinand_reset- send RESET command "0xff" to the Nand device.
 */
static void spinand_reset(struct spi_device *spi_nand)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = CMD_RESET;

	if (spinand_cmd(spi_nand, &cmd) < 0)
		pr_info("spinand reset failed!\n");

	/* elapse 1ms before issuing any other command */
	usleep_range(1000, 2000);

	if (wait_till_ready(spi_nand))
		dev_err(&spi_nand->dev, "wait timedout!\n");
	spinand_lock_block(spi_nand, BL_ALL_UNLOCKED);

	spinand_enable_ecc(spi_nand);
}

static int spinand_cmdfunc(struct mtd_info *mtd, unsigned int command,
			    int column, int page)
{
	int ret = 0;
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct spinand_info *info = (struct spinand_info *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)info->priv;

	switch (command) {
	/*
	 * READ0 - read in first  0x800 bytes
	 */
	case NAND_CMD_READ1:
	case NAND_CMD_READ0:
		state->buf_ptr = 0;
		ret = spinand_read_page(info->spi, page, 0x0, 0x840, state->buf);
		break;
	/* READOOB reads only the OOB because no ECC is performed. */
	case NAND_CMD_READOOB:
		state->buf_ptr = 0;
		ret = spinand_read_page(info->spi, page, 0x800, 0x40, state->buf);
		break;
	case NAND_CMD_RNDOUT:
		state->buf_ptr = column;
		break;
	case NAND_CMD_READID:
		state->buf_ptr = 0;
		spinand_read_id(info->spi, state->buf);
		break;
	case NAND_CMD_PARAM:
		state->buf_ptr = 0;
		break;
	/* ERASE1 stores the block and page address */
	case NAND_CMD_ERASE1:
		spinand_erase_block(info->spi, page);
		break;
	/* ERASE2 uses the block and page address from ERASE1 */
	case NAND_CMD_ERASE2:
		break;
	/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN:
		state->col = column;
		state->row = page;
		state->buf_ptr = 0;
		break;
	/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
		ret = spinand_program_page(info->spi, state->row, state->col, state->buf_ptr, state->buf);
		break;
	case NAND_CMD_STATUS:
		spinand_get_otp(info->spi, state->buf);
		if (!(state->buf[0] & 0x80))
			state->buf[0] = 0x80;
		state->buf_ptr = 0;
		break;
	/* RESET command */
	case NAND_CMD_RESET:
		break;
	default:
		dev_err(&mtd->dev, "Unknown CMD: 0x%x\n", command);
	}

	return ret;
}

/**
 * spinand_lock_block- send write register 0x1f command to the Nand device
 *
 * Description:
 *    After power up, all the Nand blocks are locked.  This function allows
 *    one to unlock the blocks, and so it can be written or erased.
 */
static int spinand_lock_block(struct spi_device *spi_nand, u8 lock)
{
	struct spinand_cmd cmd = {0};
	int ret;
	u8 otp = 0;

	ret = spinand_get_otp(spi_nand, &otp);

	cmd.cmd = CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_BLOCK_LOCK;
	cmd.n_tx = 1;
	cmd.tx_buf = &lock;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "error %d lock block\n", ret);

	return ret;
}

/*
 * spinand_probe - [spinand Interface]
 * @spi_nand: registered device driver.
 *
 * Description:
 *   To set up the device driver parameters to make the device available.
 */
int spinand_probe(struct spi_mem *spimem, const char *node)
{
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct spinand_info *info;
	struct spinand_state *state;
	struct spi_device *spi_nand = spimem->spi;

	info  = devm_kzalloc(&spi_nand->dev, sizeof(struct spinand_info),
			     GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->spi = spi_nand;

	spinand_reset(spi_nand);
	usleep_range(250, 1000);

	state = devm_kzalloc(&spi_nand->dev, sizeof(struct spinand_state),
			     GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	info->priv	= state;
	state->buf_ptr	= 0;
	state->buf	= devm_kzalloc(&spi_nand->dev, BUFSIZE, GFP_KERNEL);
	if (!state->buf)
		return -ENOMEM;

	chip = devm_kzalloc(&spi_nand->dev, sizeof(struct nand_chip),
			    GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->ecc.mode	= NAND_ECC_HW;
	chip->ecc.size	= 0x200;
	chip->ecc.bytes	= 0x6;
	chip->ecc.steps	= 0x4;

	chip->ecc.strength = 1;
	chip->ecc.total	= chip->ecc.steps * chip->ecc.bytes;
	chip->ecc.layout = &spinand_oob_64;
	chip->ecc.read_page = spinand_read_page_hwecc;
	chip->ecc.write_page = spinand_write_page_hwecc;

	chip->priv	= info;
	chip->read_buf	= spinand_read_buf;
	chip->write_buf	= spinand_write_buf;
	chip->read_byte	= spinand_read_byte;
	chip->cmdfunc	= spinand_cmdfunc;
	chip->waitfunc	= spinand_wait;
	chip->options	|= NAND_CACHEPRG;
	chip->select_chip = spinand_select_chip;

	mtd = devm_kzalloc(&spi_nand->dev, sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd)
		return -ENOMEM;

	spi_mem_set_drvdata(spimem, spi_nand);

	mtd->priv = chip;
	mtd->oobsize = 64;

	mtd->name = "spi nand";
	if (nand_scan(mtd, 1, node))
		return -ENXIO;

	spinand_enable_ecc(spi_nand);

	spinand_lock_block(spi_nand, BL_ALL_UNLOCKED);

	return mtd_device_register(mtd, node);
}
