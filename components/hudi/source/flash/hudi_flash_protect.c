#include <stdint.h>            
#include <unistd.h>
#include <stdio.h>             
#include <stdlib.h>            
#include <string.h>            
#include <stdbool.h>            
#include <getopt.h>            
#include <fcntl.h>             
#include <sys/ioctl.h>
#include <linux/types.h>
                               
#ifdef __HCRTOS__
#include <hcuapi/spidev.h>
#include <kernel/lib/console.h>
#else
#include <linux/spi/spidev.h>
#endif

/*
 * Lock a region of the flash. Compatible with ST Micro and similar flash.
 * Supports the block protection bits BP{0,1,2}/BP{0,1,2,3} in the status
 * register
 * (SR). Does not support these features found in newer SR bitfields:
 *   - SEC: sector/block protect - only handle SEC=0 (block protect)
 *   - CMP: complement protect - only support CMP=0 (range is not complemented)
 *
 * Support for the following is provided conditionally for some flash:
 *   - TB: top/bottom protect
 *
 * Sample table portion for 16MB flash (BYTe BY25Q128ES):
 * 具体参考所使用的flash的文档。
 *
 * SEC/BP3 | TB/BP3 |  BP2  |  BP1  |  BP0  |  Prot Length  | Protected Portion
 *  --------------------------------------------------------------------------
 *  CMP=0
 *  --------------------------------------------------------------------------
 *    X    |   X    |   0   |   0   |   0   |  NONE         | NONE
 *    0    |   0    |   0   |   0   |   1   |  256 KB       | Upper 1/64
 *    0    |   0    |   0   |   1   |   0   |  512 KB       | Upper 1/32
 *    0    |   0    |   0   |   1   |   1   |  1 MB         | Upper 1/16
 *    0    |   0    |   1   |   0   |   0   |  2 MB         | Upper 1/8
 *    0    |   0    |   1   |   0   |   1   |  4 MB         | Upper 1/4
 *    0    |   0    |   1   |   1   |   0   |  8 MB         | Upper 1/2
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    0    |   1    |   0   |   0   |   1   |  256 KB       | Lower 1/64
 *    0    |   1    |   0   |   1   |   0   |  512 KB       | Lower 1/32
 *    0    |   1    |   0   |   1   |   1   |  1 MB         | Lower 1/16
 *    0    |   1    |   1   |   0   |   0   |  2 MB         | Lower 1/8
 *    0    |   1    |   1   |   0   |   1   |  4 MB         | Lower 1/4
 *    0    |   1    |   1   |   1   |   0   |  8 MB         | Lower 1/2
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    X    |   X    |   1   |   1   |   1   |  16 MB        | ALL
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    1    |   0    |   0   |   0   |   1   |  4 KB         | Top Block
 *    1    |   0    |   0   |   1   |   0   |  8 KB         | Top Block
 *    1    |   0    |   0   |   1   |   1   |  16 KB        | Top Block
 *    1    |   0    |   1   |   0   |   x   |  32 KB        | Top Block
 *    1    |   0    |   1   |   1   |   1   |  32 KB        | Top Block
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    1    |   1    |   0   |   0   |   1   |  4 KB         | Bottom Block
 *    1    |   1    |   0   |   1   |   0   |  8 KB         | Bottom Block
 *    1    |   1    |   0   |   1   |   1   |  16 KB        | Bottom Block
 *    1    |   1    |   1   |   0   |   x   |  32 KB        | Bottom Block
 *    1    |   1    |   1   |   1   |   1   |  32 KB        | Bottom Block
 *
 *  ==========================================================================
 *
 *  CMP=1
 *  --------------------------------------------------------------------------
 *    X    |   X    |   0   |   0   |   0   |  NONE         | NONE
 *    0    |   0    |   0   |   0   |   1   |  16128 KB     | Lower 63/64
 *    0    |   0    |   0   |   1   |   0   |  15872 KB     | Lower 31/32
 *    0    |   0    |   0   |   1   |   1   |  15 MB        | Lower 15/16
 *    0    |   0    |   1   |   0   |   0   |  14 MB        | Lower 7/8
 *    0    |   0    |   1   |   0   |   1   |  12 MB        | Lower 3/4
 *    0    |   0    |   1   |   1   |   0   |  8 MB         | Lower 1/2
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    0    |   1    |   0   |   0   |   1   |  16128 KB     | Upper 63/64
 *    0    |   1    |   0   |   1   |   0   |  15872 KB     | Upper 31/32
 *    0    |   1    |   0   |   1   |   1   |  15 MB        | Upper 15/16
 *    0    |   1    |   1   |   0   |   0   |  14 MB        | Upper 7/8
 *    0    |   1    |   1   |   0   |   1   |  12 MB        | Upper 3/4
 *    0    |   1    |   1   |   1   |   0   |  8 MB         | Upper 1/2
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    X    |   X    |   1   |   1   |   1   |  NONE         | NONE
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    1    |   0    |   0   |   0   |   1   |  16380 KB     | L-4095/4096
 *    1    |   0    |   0   |   1   |   0   |  16376 KB     | L-2047/2048
 *    1    |   0    |   0   |   1   |   1   |  16368 KB     | L-1023/1024
 *    1    |   0    |   1   |   0   |   x   |  16352 KB     | L-511/512
 *    1    |   0    |   1   |   1   |   0   |  16352 KB     | L-511/512
 *  ------ |------- |-------|-------|-------|---------------|-----------------
 *    1    |   1    |   0   |   0   |   1   |  16380 KB     | U-4095/4096
 *    1    |   1    |   0   |   1   |   0   |  16376 KB     | U-2047/2048
 *    1    |   1    |   0   |   1   |   1   |  16368 KB     | U-1023/1024
 *    1    |   1    |   1   |   0   |   x   |  16352 KB     | U-511/512
 *    1    |   1    |   1   |   1   |   0   |  16352 KB     | U-511/512
 */

#ifdef __HCRTOS__
	static const char *device = "/dev/spidev0";
#else
	static const char *device = "/dev/spidev32766.1";
#endif

static uint8_t bits = 8;
static uint32_t speed = 500000;

int hc_norflash_read_register(uint8_t cmd, uint8_t *data)
{
	int ret, fd;
	uint8_t tx[1] = {0};
	uint8_t *rx = data;
	tx[0] = cmd;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device\n");
		return -1;
	}

	struct spi_ioc_transfer xfer[2] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 1,
			.speed_hz = speed,
			.bits_per_word = bits,
		},{
			.tx_buf = (unsigned long)NULL,
			.rx_buf = (unsigned long)rx,
			.len = 1,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
	if (ret < 1) {
		printf("can't send  spi message\n");
		return -1;
	}

	close(fd);
	return 0;
}

int hc_norflash_send_one_cmd(uint8_t cmd)
{
	int ret, fd;
	uint8_t tx[1] = {0};
	tx[0] = cmd;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device\n");
		return -1;
	}

	struct spi_ioc_transfer xfer[1] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 1,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
	if (ret < 1) {
		printf("can't send  spi message\n");
		return -1;
	}

	close(fd);
	return 0;
}

int hc_norflash_write_register(uint8_t cmd, uint8_t *data)
{
	int ret, fd;
	uint8_t tx[2] = {0};
	tx[0] = cmd;
	tx[1] = *data;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device\n");
		return -1;
	}

	struct spi_ioc_transfer xfer[1] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = sizeof(tx),
			.speed_hz = speed,
			.bits_per_word = bits,
		}
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
	if (ret < 1) {
		printf("can't send  spi message\n");
		return -1;
	}

	close(fd);
	return 0;
}

int hudi_nor_flash_set_protect(unsigned char cmp, unsigned char bp) {
	int ret = 0;
	uint8_t register0 = 0;
	uint8_t register1 = 0;
	int i = 0;
	int fdlock = -1;

	//	hudi_flash_lock();

#ifdef __HCRTOS__
	fdlock = open("/dev/sf_prodect", O_RDWR);
	if (fdlock < 0) {
		fdlock = open("/dev/sf_protect", O_RDWR);
		if (fdlock < 0) {
			perror("Open lock");
			//	hudi_flash_unlock();
			return -1;
		}
	}
#else
	fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
	if (fdlock < 0) {
		perror("Open lock");
		//	hudi_flash_unlock();
		return -1;
	}

	if (write(fdlock, "lock", 4) <= 0) {
		perror("Write lock");
		close(fdlock);
		//	hudi_flash_unlock();
		return -1;
	}
#endif

	/* RESET FLASH */
	ret = hc_norflash_send_one_cmd(0x66);
	ret = hc_norflash_send_one_cmd(0x99);

	/* set cmp */
	/* check idle */
	ret = hc_norflash_read_register(0x05, &register0);
	while (register0 & 0x03) {
		usleep(5 * 1000);
		ret = hc_norflash_read_register(0x05, &register0);
		i++;
		if (i == 1000) {
			printf("hudi flash protect timeout\n");
			//	hudi_flash_unlock();
			return -1;
		}
	}

	/* write enable */
	ret = hc_norflash_send_one_cmd(0x06);

	ret = hc_norflash_read_register(0x35, &register1);

	if (cmp)
		register1 |= 0x40;
	else
		register1 &= ~0x40;

	ret = hc_norflash_write_register(0x31, &register1);
	ret = hc_norflash_read_register(0x05, &register0);
	i = 0;
	while (register0 & 0x03) {		/* wait to idle */
		usleep(5 * 1000);
		ret = hc_norflash_read_register(0x05, &register0);
		i++;
		if (i == 10000) {
			printf("hudi flash protect timeout\n");
			//	hudi_flash_unlock();
			return -1;
		}
	}

	ret = hc_norflash_read_register(0x35, &register1);

	/* write disable */
	ret = hc_norflash_send_one_cmd(0x04);

	/* set lock area */
	bp <<= 2;
	bp &= 0x7C;

	/* write enable */
	ret = hc_norflash_send_one_cmd(0x06);

	/* write lock area */
	ret = hc_norflash_write_register(0x01, &bp);

	ret = hc_norflash_read_register(0x05, &register0);
	i = 0;
	while (register0 & 0x03) {		/* wait to idle */
		usleep(5 * 1000);
		ret = hc_norflash_read_register(0x05, &register0);
		i++;
		if (i == 1000) {
			printf("hudi flash protect timeout\n");
			//	hudi_flash_unlock();
			return -1;
		}
	}

	ret = hc_norflash_read_register(0x05, &register0);

	/* write disable */
	ret = hc_norflash_send_one_cmd(0x04);

#ifdef __HCRTOS__
	close(fdlock);
#else
	if (write(fdlock, "unlock", 6) <= 0) {
		perror("Write unlock");
	}
	close(fdlock);
#endif
	//	hudi_flash_unlock();

	return ret;
}

int hudi_nor_flash_get_protect(unsigned char *cmp, unsigned char *bp)
{
	int ret = 0;
	uint8_t register0 = 0;
	uint8_t register1 = 0;
	int i = 0;
	int fdlock = -1;
	//	hudi_flash_lock();

#ifdef __HCRTOS__
	fdlock = open("/dev/sf_prodect", O_RDWR);
	if (fdlock < 0) {
		fdlock = open("/dev/sf_protect", O_RDWR);
		if (fdlock < 0) {
			perror("Open lock");
			//	hudi_flash_unlock();
			return -1;
		}
	}
#else
	fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
	if (fdlock < 0) {
		perror("Open lock");
		//	hudi_flash_unlock();
		return -1;
	}

	if (write(fdlock, "lock", 4) <= 0) {
		perror("Write lock");
		close(fdlock);
		//	hudi_flash_unlock();
		return -1;
	}
#endif

	/* RESET FLASH */
	ret = hc_norflash_send_one_cmd(0x66);
	ret = hc_norflash_send_one_cmd(0x99);

	ret = hc_norflash_read_register(0x05, &register0);
	while (register0 & 0x03) {
		ret = hc_norflash_read_register(0x05, &register0);
		usleep(5 * 1000);
		i++;
		if (i == 1000) {
			printf("hudi flash protect timeout\n");
			//	hudi_flash_unlock();
			return -1;
		}
	}
	ret = hc_norflash_read_register(0x05, &register0);
	*bp = register0 >> 2;

	i = 0;
	ret = hc_norflash_read_register(0x05, &register0);
	while (register0 & 0x03) {
		ret = hc_norflash_read_register(0x05, &register0);
		usleep(5 * 1000);
		i++;
		if (i == 1000) {
			printf("hudi flash protect timeout\n");
			//	hudi_flash_unlock();
			return -1;
		}
	}
	ret = hc_norflash_read_register(0x35, &register1);
	*cmp = register1 >> 6;

#ifdef __HCRTOS__
	close(fdlock);
#else
	if (write(fdlock, "unlock", 6) <= 0) {
		perror("Write unlock");
	}
	close(fdlock);
#endif
	//	hudi_flash_unlock();

	return ret;
}

#if 0
#ifdef __HCRTOS__
int flash_protect(int argc, char * argv[])
#else
int main(int argc, char **argv)
#endif
{
	char ch;
	long tmp;
	opterr = 0;
	optind = 0;

	int bp = 0;
	int cmp = 0;

	while ((ch = getopt(argc, argv, "hi:l:")) != EOF) {
		switch (ch) {
			case 'i':
				tmp = strtoll(optarg, NULL, 10);
				bp = tmp;
				break;
			case 'l':
				tmp = strtoll(optarg, NULL, 10);
				cmp = tmp;
				break;
			default:
				printf("Invalid parameter %c\r\n", ch);
				return -1;
		}
	}

	int ret = 0;

	printf("bp = 0x%x,cmp = 0x%x\n", bp, cmp);
	/*
	 * 例如我要锁定flash的低15MB空间不让访问;
	 * 参考表格flash文档的表格，我应该传入cmp = 0x1,bp = 0x3;
	 */
	ret = hudi_nor_flash_set_protect(cmp, bp);

	cmp = 0;
	bp = 0;

	ret = hudi_nor_flash_get_protect((unsigned char *)&cmp, (unsigned char *)&bp);
	printf("bp = 0x%x,cmp = 0x%x\n", bp, cmp);

	return ret;
}

#ifdef __HCRTOS__
CONSOLE_CMD(flash_protect, NULL, flash_protect, CONSOLE_CMD_MODE_SELF, "test key-adc function app of 1512")
#endif
#endif
