#define LOG_TAG "main"

#include <generated/br2_autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <sys/poll.h>
#include <kernel/module.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <cpu_func.h>
#include <hcuapi/sysdata.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <sys/ioctl.h>
#include <hcuapi/gpio.h>
#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/ld.h>
#include <nuttx/mtd/mtd.h>
#include <linux/mtd/mtd.h>
#include <sys/ioctl.h>
#include <linux/minmax.h>
#include <hcfota.h>
#include <kernel/lib/crc32.h>
#include <hcuapi/efuse.h>
#include <hudi/hudi_com.h>
#include <hudi/hudi_flash.h>

#define SpiNorEnCS0	BIT(0)
#define SpiNandEnCS0	BIT(1)
#define SpiNandEnCS1	BIT(2)
#define NandEn		BIT(3)
#define EmmcEnV20Intf0	BIT(4)
#define EmmcEnV20Intf1	BIT(5)
#define EmmcEnV30	BIT(6)

#define CONNECTTYPE_UART 0xa5a55a5a
#define CONNECTTYPE_USB  0xaaaa5555
#define FOTA_PAYLOAD_MAGIC 0xabcd1234
#define FOTA_DUMP_MAGIC 0x1234abcd
static uint32_t ConnectType = 0;
static uint32_t FotaSize = 0;
static uint32_t FotaPayloadMagic = 0;
static void *pFota = NULL;

enum UPG_RPC_ID {
	UPG_RPC_READ_FOTA = 0x1000,
	UPG_RPC_REPORT_PROGRESS,
	UPG_RPC_REPORT_RESULT,
	UPG_RPC_DUMP_EFUSE,
	UPG_RPC_DUMP_OTP_NOR,
	UPG_RPC_DUMP_OTP_SPINAND,
	UPG_RPC_DUMP_NOR,
	UPG_RPC_DUMP_NAND,
	UPG_RPC_DUMP_SDMMC,
	UPG_RPC_SEND_DUMP_RESULT,
	UPG_RPC_SEND_DUMP_RESULT_DONE,
	UPG_RPC_PROGRAM_NOR,
	UPG_RPC_PROGRAM_NAND,
	UPG_RPC_PROGRAM_SDMMC,
	UPG_RPC_TEST_MEMTESTER,
	UPG_RPC_SEND_LOG,
};

enum UPG_RPC_DIR {
	UPG_RPC_H2D = 1, /* host to target device */
	UPG_RPC_D2H= 2,  /* target device to host */
};

enum UPG_RPC_STATUS {
	UPG_RPC_STATUS_SETUP = 1,
	UPG_RPC_STATUS_DONE = 2,
};

typedef volatile struct {
	uint32_t dir;
	uint32_t id;
	uint32_t status;
} upg_rpc_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t offset;
	uint32_t buf;
	uint32_t nbytes;
	uint32_t crc;
} d2h_read_fota_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t progress;
} h2d_get_progress_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t result;
} h2d_get_result_arg_t;

struct dump_otp {
	unsigned char uid[16];
	unsigned char bank1[256];
	unsigned char bank2[256];
	unsigned char bank3[256];
};

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t offset;
	uint32_t size;
} h2d_dump_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t total;
	uint32_t offset;
	uint32_t buf;
	uint32_t nbytes;
	uint32_t crc;
} d2h_send_dump_result_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t result;
} d2h_send_result_arg_t;

typedef volatile struct {
	upg_rpc_arg_t rpc;
	uint32_t len;
	char log[0];
} d2h_send_log_arg_t;

static uint32_t upg_progress = 0;
static uint32_t upg_result = -1;

static void app_main(void *pvParameters);
static void usb_exchange_backend(void *pvParameters);
static void uart_exchange_backend(void *pvParameters);
static void upg_rpc_backend(void *pvParameters);
extern int soc_printf(const char *format, ...);
#define upg_printf(...)                                                        \
	do {                                                                   \
		printf(__VA_ARGS__);                                           \
		if (ConnectType != CONNECTTYPE_UART &&                         \
		    ConnectType != CONNECTTYPE_USB)                            \
			soc_printf(__VA_ARGS__);                               \
	} while (0)

#if 0
static int ____putchar(int c)
{
	REG8_WRITE(0xB8818600, (char)(c));
	while (!(REG8_READ(0xB8818605) & BIT5))
		;
	return 0;
}

static int __puts(const char *s)
{
	while ((*s) != '\0') {
		____putchar(*s);
		s++;
	}
	return 0;
}
#endif

const char *fdt_get_sysmem_path(void)
{
#if 0
	uint32_t ddr_size = REG32_GET_FIELD2(0xb8801000, 0, 3);

	ddr_size = 16 << (ddr_size);
	if (ddr_size <= 16)
		return "/hcrtos/memory-mapping/sysmem4m";

	return "/hcrtos/memory-mapping/sysmem";
#else
	return "/hcrtos/memory-mapping/sysmem4m";
#endif
}

static void fdt_fixup(void *fota, uint32_t size)
{
	int np;
	void *dtb = get_fdt();
	struct hcfota_header head = { 0 };

	memcpy(&head, fota, sizeof(head));

	if (head.spinor_en_cs0) {
		upg_printf("Spi Nor device is enabled on CS0\n");
		np = fdt_path_offset(dtb, "/hcrtos/sfspi/spi_nor_flash");
		if (np >= 0) {
			fdt_setprop_u32(dtb, np, "reg", 0);
			fdt_setprop(dtb, np, "status", "okay", 4);
		}
	}

	if (head.spinand_en_cs0) {
		upg_printf("Spi Nand device is enabled on CS0\n");
		np = fdt_path_offset(dtb, "/hcrtos/sfspi/spi_nand_flash");
		if (np >= 0) {
			fdt_setprop_u32(dtb, np, "reg", 0);
			fdt_setprop(dtb, np, "status", "okay", 4);
		}
	}

	if (head.spinand_en_cs1) {
		upg_printf("Spi Nand device is enabled on CS1\n");
		np = fdt_path_offset(dtb, "/hcrtos/sfspi/spi_nand_flash");
		if (np >= 0) {
			fdt_setprop_u32(dtb, np, "reg", 1);
			fdt_setprop(dtb, np, "status", "okay", 4);
		}
	}

	if (head.nand_en) {
		if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
			upg_printf("Nand device is enabled on H16xx\n");
			np = fdt_path_offset(dtb, "/hcrtos/nand");
			if (np >= 0) {
				fdt_setprop(dtb, np, "status", "okay", 4);
			}
		} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
			upg_printf("Trying to enable Nand device on H15xx is not supported\n");
		}
	}

	if (head.emmc_v20_left_en && ConnectType != CONNECTTYPE_UART) {
		if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
			upg_printf("Emmc/SD device is enabled on H15xx interface-0\n");
			np = fdt_path_offset(dtb, "/hcrtos/mmcv20intf0");
			if (np >= 0) {
				fdt_setprop(dtb, np, "status", "okay", 4);
			}
		}
	}

	if (head.emmc_v20_top_en) {
		if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
			upg_printf("Emmc/SD device is enabled on H15xx interface-1\n");
			np = fdt_path_offset(dtb, "/hcrtos/mmcv20intf1");
			if (np >= 0) {
				fdt_setprop(dtb, np, "status", "okay", 4);
			}
		}
	}

	if (head.emmc_v30_en) {
		if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
			upg_printf("Emmc/SD device is enabled on H16xx\n");
			np = fdt_path_offset(dtb, "/hcrtos/mmcv30");
			if (np >= 0) {
				fdt_setprop(dtb, np, "status", "okay", 4);
			}
		}
	}
}

static int read_otp(struct dump_otp *otp, hudi_flash_type_e type)
{
	hudi_handle hdl = NULL;
	unsigned int uid_len = 16;
	int i;

	if (0 != hudi_flash_open(&hdl, type)) {
		printf("hudi flash open fail\n");
		return -1;
	}

	hudi_flash_uid_read(hdl, otp->uid, &uid_len);
	hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG1, 0, otp->bank1, 256);
	hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG2, 0, otp->bank2, 256);
	hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, otp->bank3, 256);
	hudi_flash_close(hdl);

	return 0;
}

int main(void)
{
	FotaSize	= *((unsigned long *)0x800001F0);
	pFota		= (void *)(*((unsigned long *)0x800001F4));
	ConnectType	= *((unsigned long *)0x800001F8);
	FotaPayloadMagic= *((unsigned long *)0x800001EC);
	*((unsigned long *)0x800001FC) = 0xeeeeaaaa; /* set magic number to notify the updater is running */
	void *strap_pin_addr = (void *)&STRAP_PIN_CTRL;
	REG32_CLR_BIT(strap_pin_addr, BIT17); //enable sflash cs/clk/mosi/miso pin
	if (ConnectType == CONNECTTYPE_UART) {
#ifdef CONFIG_SOC_HC16XX
		REG32_CLR_BIT(strap_pin_addr, BIT18); //enable uart0 strap pin
#elif defined(CONFIG_SOC_HC15XX)
		REG32_CLR_BIT(strap_pin_addr, BIT18); //enable uart0 strap pin for QFN100 and QFN120_RGB
		REG32_CLR_BIT(strap_pin_addr, BIT19); //enable uart0 strap pin for QFN68 and QFN128_HDMI
#endif
	}

	upg_printf("HCFOTA bin found at 0x%08lx, bin size %ld\n", (uint32_t)pFota, FotaSize);
	fdt_fixup(pFota, FotaSize);

	if (ConnectType == CONNECTTYPE_USB || ConnectType == CONNECTTYPE_UART) {
		xTaskCreate(upg_rpc_backend, (const char *)"rpc backend",
			    configTASK_STACK_DEPTH * 2, NULL,
			    portPRI_TASK_NORMAL, NULL);
	}

	if (ConnectType == CONNECTTYPE_USB) {
		xTaskCreate(usb_exchange_backend, (const char *)"usb backend",
			    configTASK_STACK_DEPTH * 2, NULL,
			    portPRI_TASK_NORMAL, NULL);
	}

	if (ConnectType == CONNECTTYPE_UART) {
		xTaskCreate(uart_exchange_backend, (const char *)"uart backend",
			    configTASK_STACK_DEPTH * 2, NULL,
			    portPRI_TASK_NORMAL, NULL);
	}

	xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH*2,
			NULL, portPRI_TASK_NORMAL, NULL);

	vTaskStartScheduler();

	abort();
	return 0;
}

static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
	if (event == HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS) {
		upg_printf("Upgrading: %d%%\r\n", (int)param);
		upg_progress = (uint32_t)param;
	}

	return 0;
}

static ssize_t hcfota_request_read(int offset, char *buf, int nbytes, unsigned long usrdata)
{
	uint32_t crc;
	d2h_read_fota_arg_t *arg = (d2h_read_fota_arg_t *)0x80000100;
	d2h_read_fota_arg_t tmp = { 0 };

	arg->rpc.status = 0;
	memset((void *)arg, 0, sizeof(*arg));
	tmp.crc = 0xffffffff;
	tmp.rpc.dir = UPG_RPC_D2H;
	tmp.rpc.id = UPG_RPC_READ_FOTA;
	tmp.offset = (uint32_t)offset;
	tmp.buf = (uint32_t)buf;
	tmp.nbytes = (uint32_t)nbytes;
	tmp.rpc.status = UPG_RPC_STATUS_SETUP;
	crc = crc32(0, (const uint8_t *)&tmp, sizeof(tmp));

	while (1) {
		arg->crc = tmp.crc;
		arg->rpc.dir = tmp.rpc.dir;
		arg->rpc.id = tmp.rpc.id;
		arg->offset = tmp.offset;
		arg->buf = tmp.buf;
		arg->nbytes = tmp.nbytes;
		arg->rpc.status = tmp.rpc.status;
		while (arg->rpc.status != UPG_RPC_STATUS_DONE)
			usleep(1000);

		crc = crc32(crc, (const uint8_t *)buf, arg->nbytes);
		if (arg->crc == 0xffffffff || arg->crc == crc) {
			break;
		}
	}

	return arg->nbytes;
}

static size_t iobread(void *iob, void* ptr, size_t size, off_t offset)
{
	return hcfota_request_read((int)offset, ptr, size, 0);
}

static ssize_t __hcfota_send_dump_result(int total, int offset, char *buf, int nbytes)
{
	uint32_t crc;
	d2h_send_dump_result_arg_t *arg = (d2h_send_dump_result_arg_t *)0x80000100;
	d2h_send_dump_result_arg_t tmp = { 0 };

	arg->rpc.status = 0;
	memset((void *)arg, 0, sizeof(*arg));
	tmp.crc = 0xffffffff;
	tmp.rpc.dir = UPG_RPC_D2H;
	tmp.rpc.id = UPG_RPC_SEND_DUMP_RESULT;
	tmp.total = (uint32_t)total;
	tmp.offset = (uint32_t)offset;
	tmp.buf = (uint32_t)buf;
	tmp.nbytes = (uint32_t)nbytes;
	tmp.rpc.status = UPG_RPC_STATUS_SETUP;
	crc = crc32(0, (const uint8_t *)&tmp, sizeof(tmp));

	while (1) {
		arg->crc = tmp.crc;
		arg->rpc.dir = tmp.rpc.dir;
		arg->rpc.id = tmp.rpc.id;
		arg->total = tmp.total;
		arg->offset = tmp.offset;
		arg->buf = tmp.buf;
		arg->nbytes = tmp.nbytes;
		arg->rpc.status = tmp.rpc.status;
		while (arg->rpc.status != UPG_RPC_STATUS_DONE)
			usleep(1000);

		crc = crc32(crc, (const uint8_t *)buf, arg->nbytes);
		if (arg->crc == 0xffffffff || arg->crc == crc) {
			break;
		}
	}

	return arg->nbytes;
}

static ssize_t hcfota_send_dump_result(int total, int offset, char *buf, int nbytes)
{
	int segment = 0;
	int remain = nbytes;
	while (remain > 0) {
		if (remain > 512)
			segment = 512;
		else
			segment = remain;

		__hcfota_send_dump_result(total, offset, buf, segment);
		remain -= segment;
		offset += segment;
		buf += segment;
	}
	return nbytes;
}

static void hcfota_send_dump_result_done(int result)
{
	d2h_send_result_arg_t *arg = (d2h_send_result_arg_t *)0x80000100;

	upg_printf(">>>>%s(%d)\r\n", __func__, result);

	arg->result = (uint32_t)result;
	arg->rpc.dir = UPG_RPC_D2H;
	arg->rpc.id = UPG_RPC_SEND_DUMP_RESULT_DONE;
	arg->rpc.status = UPG_RPC_STATUS_SETUP;
	while (arg->rpc.status != UPG_RPC_STATUS_DONE)
		usleep(1000);

	upg_printf("<<<<%s(%d)\r\n", __func__, result);
}

static void hcfota_dump_efuse(void)
{
	int rc = 0;
	struct hc_efuse_bit_map bitmap = { 0 };
	upg_printf("dump efuse cmd get\n");
	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		int fd = open("/dev/efuse", O_RDWR);
		if (fd > 0) {
			ioctl(fd, EFUSE_DUMP, (uint32_t)&bitmap);
			close(fd);
			upg_printf("dump efuse success\n");
		} else {
			rc = -ENODEV;
			upg_printf("open /dev/efuse failed\n");
		}
	} else {
		rc = -ENOTSUP;
		upg_printf("Chipset not support efuse\n");
	}
	hcfota_send_dump_result(sizeof(bitmap), 0, (char *)&bitmap, sizeof(bitmap));
	hcfota_send_dump_result_done(rc);
}

static void hcfota_dump_otp_nor(void)
{
	int rc;
	upg_printf("dump OTP-NOR cmd get\n");
	struct dump_otp *otp = malloc(sizeof(*otp));
	memset(otp, 0, sizeof(*otp));
	rc = read_otp(otp, HUDI_FLASH_TYPE_NOR);
	hcfota_send_dump_result(sizeof(*otp), 0, (char *)otp, sizeof(*otp));
	hcfota_send_dump_result_done(rc);
	free(otp);
}

static void hcfota_dump_otp_spinand(void)
{
	int rc;
	upg_printf("dump OTP-SPINAND cmd get\n");
	struct dump_otp *otp = malloc(sizeof(*otp));
	memset(otp, 0, sizeof(*otp));
	rc = read_otp(otp, HUDI_FLASH_TYPE_NAND);
	hcfota_send_dump_result(sizeof(*otp), 0, (char *)otp, sizeof(*otp));
	hcfota_send_dump_result_done(rc);
	free(otp);
}

#define MAX_TRANSFER_SIZE 0x100000
static void hcfota_dump_nor(uint32_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nor");
	int rc = 0;
	char *buf;

	if (mtd == NULL) {
		hcfota_send_dump_result_done(-ENODEV);
		return;
	}

	buf = malloc(MAX_TRANSFER_SIZE);
	if (buf == NULL) {
		hcfota_send_dump_result_done(-ENOMEM);
		return;
	}

	uint32_t rd = 0;
	uint32_t remain;
	int segment;
	while (rd < len) {
		remain = len - rd;
		if (remain > MAX_TRANSFER_SIZE) {
			segment = MAX_TRANSFER_SIZE;
		} else {
			segment = remain;
		}
		rc = MTD_READ(mtd, offset, segment, buf);
		if (rc != segment) {
			rc = -EIO;
			break;
		} else {
			rc = 0;
		}

		hcfota_send_dump_result(len, rd, buf, segment);
		offset += segment;
		rd += segment;
	}
	free(buf);
	hcfota_send_dump_result_done(rc);
}

static int hcfota_program_nor(uint32_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nor");
	uint32_t src_remaining, dst_remaining, toff, erase_off, rem, n, mask, copied;
	int byteswrite = 0;
	off_t startblock;
	size_t nblocks;
	off_t off;
	ssize_t rc;
	unsigned char *buf, *buf2;
	int all_ff;
	off_t iob_off = 0;

	if (!mtd) {
		return -ENODEV;
	}

	if (mtd->erasesize > 0x10000)
		n = mtd->erasesize;
	else
		n = 0x10000;

	buf = malloc(n);
	if (!buf) {
		n >>= 1;
		if (n >= mtd->erasesize)
			buf = malloc(n);
	}
	if (!buf)
		return -ENOMEM;
	buf2 = malloc(n);
	if (!buf2) {
		free(buf);
		return -ENOMEM;
	}

	mask = n - 1;

	src_remaining = len;
	dst_remaining = len;
	erase_off = (uint32_t)offset;
	toff = (uint32_t)offset;

	/* Write the leading fragment of un-aligned erasesize */
	if (toff & (mtd->erasesize - 1)) {
		rem = toff & (mtd->erasesize - 1);
		off = (off_t)(toff - rem);

		startblock = off / mtd->writesize;
		nblocks = mtd->erasesize / mtd->writesize;

		rc = MTD_BREAD(mtd, startblock, nblocks, buf);
		if (rc != (int)nblocks) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		copied = min3(src_remaining, dst_remaining, mtd->erasesize - rem);

		if (iobread(NULL, buf2, copied, iob_off) != copied) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		if (copied > 0)
			memcpy(buf + rem, buf2, copied);
		startblock = off / mtd->erasesize;
		nblocks = 1;
		rc = MTD_ERASE(mtd, startblock, nblocks);
		if (rc != 0) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		rc = MTD_WRITE(mtd, off, rem + copied, buf);
		if (rc != (int)(rem + copied)) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		if (dst_remaining < (mtd->erasesize - rem)) {
			int rem2 = rem + dst_remaining;
			int cp = mtd->erasesize - rem2;
			rc = MTD_WRITE(mtd, off + rem2, cp, buf + rem2);
			if (rc != cp) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}
			rem2 = (dst_remaining - copied);
			dst_remaining -= rem2;
			erase_off += rem2;
		} else {
			dst_remaining -= (mtd->erasesize - rem - copied);
			erase_off += (mtd->erasesize - rem - copied);
		}

		toff += copied;
		iob_off += copied;
		src_remaining -= copied;
		dst_remaining -= copied;
		erase_off += copied;
		byteswrite += copied;
	}

	/* Write the continuous blocks in size n */
	while (src_remaining >= n && dst_remaining >= n) {
		startblock = (off_t)toff / mtd->writesize;
		nblocks = n / mtd->writesize;
		rc = MTD_BREAD(mtd, startblock, nblocks, buf2);
		if (rc != (int)nblocks) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		if (iobread(NULL, buf, n, iob_off) != n) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		if (memcmp(buf, buf2, n)) {
			startblock = (off_t)toff / mtd->erasesize;
			nblocks = n / mtd->erasesize;
			rc = MTD_ERASE(mtd, startblock, nblocks);
			if (rc != 0) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}

			startblock = (off_t)toff / mtd->writesize;
			nblocks = n / mtd->writesize;

			rc = MTD_BWRITE(mtd, startblock, nblocks, buf);
			if (rc != (int)nblocks) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}
		}

		toff += n;
		iob_off += n;
		src_remaining -= n;
		dst_remaining -= n;
		erase_off += n;
		byteswrite += n;
	}

	/* Write the tail fragment of un-aligned n */
	if (src_remaining > 0 && dst_remaining > 0) {
		n = min(mtd->size - toff, n);
		startblock = (off_t)toff / mtd->writesize;
		nblocks = n / mtd->writesize;
		rc = MTD_BREAD(mtd, startblock, nblocks, buf);
		if (rc != (int)nblocks) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		copied = min(src_remaining, dst_remaining);
		if (iobread(NULL, buf, copied, iob_off) != copied) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		startblock = (off_t)toff / mtd->erasesize;
		nblocks = n / mtd->erasesize;
		rc = MTD_ERASE(mtd, startblock, nblocks);
		if (rc != 0) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		rc = MTD_WRITE(mtd, (off_t)toff, copied, buf);
		if (rc != (int)copied) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		rem = min(dst_remaining, n);
		if (rem != n) {
			int cp = n - rem;
			rc = MTD_WRITE(mtd, (off_t)toff + rem, cp, buf + rem);
			if (rc != cp) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}
			cp = dst_remaining - copied;
			dst_remaining -= cp;
			erase_off += cp;
		} else {
			dst_remaining -= (n - copied);
			erase_off += (n - copied);
		}

		toff += copied;
		iob_off += copied;
		src_remaining -= copied;
		dst_remaining -= copied;
		erase_off += copied;
		byteswrite += copied;
	}

	while (dst_remaining >= n) {
		startblock = (off_t)erase_off / mtd->erasesize;
		nblocks = n / mtd->erasesize;
		rc = MTD_BREAD(mtd, startblock, nblocks, buf2);
		if (rc != (int)nblocks) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}
		all_ff = 1;
		for (unsigned i = 0; i < n; i++) {
			if (buf2[i] != 0xff) {
				all_ff = 0;
				break;
			}
		}

		if (all_ff == 0) {
			rc = MTD_ERASE(mtd, startblock, nblocks);
			if (rc != 0) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}
		}
		dst_remaining -= n;
		erase_off += n;
	}

	if (dst_remaining > 0) {
		n = min(mtd->size - erase_off, n);
		startblock = (off_t)erase_off / mtd->writesize;
		nblocks = n / mtd->writesize;
		rc = MTD_BREAD(mtd, startblock, nblocks, buf2);
		if (rc != (int)nblocks) {
			free(buf);
			free(buf2);
			return HCFOTA_ERR_UPGRADE;
		}

		all_ff = 1;
		for (unsigned i = 0; i < dst_remaining; i++) {
			if (buf2[i] != 0xff) {
				all_ff = 0;
				break;
			}
		}

		if (all_ff == 0) {
			startblock = (off_t)erase_off / mtd->erasesize;
			nblocks = n / mtd->erasesize;
			rc = MTD_ERASE(mtd, startblock, nblocks);
			if (rc != 0) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}

			rc = MTD_WRITE(mtd, (off_t)erase_off + dst_remaining, n - dst_remaining, buf2 + dst_remaining);
			if (rc != (int)(n - dst_remaining)) {
				free(buf);
				free(buf2);
				return HCFOTA_ERR_UPGRADE;
			}
		}
	}

	free(buf);
	free(buf2);

	return 0;
}

static void hcfota_dump_nand(uint32_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nand");
	int bytesread = 0;
	off_t startblock;
	size_t nblocks;
	uint32_t erasesize;
	uint32_t total;
	ssize_t rc;
	char *buf;

	if (mtd == NULL) {
		hcfota_send_dump_result_done(-ENODEV);
		return;
	}

	buf = malloc(MAX_TRANSFER_SIZE);
	if (buf == NULL) {
		hcfota_send_dump_result_done(-ENOMEM);
		return;
	}

	erasesize = mtd->erasesize;
	len = ((len + erasesize - 1) / erasesize) * erasesize;
	total = len;

	/* Skip badblocks if there is any */
	while (MTD_BLOCK_ISBAD(mtd, offset / erasesize) && len > 0) {
		offset += erasesize;
		len -= erasesize;
		total -= erasesize;
	}

	while (len > 0) {
		if (MTD_BLOCK_ISBAD(mtd, offset / erasesize)) {
			offset += erasesize;
			len -= erasesize;
			total -= erasesize;
			continue;
		}

		startblock = offset / mtd->writesize;
		nblocks = erasesize / mtd->writesize;

		rc = MTD_BREAD(mtd, startblock, nblocks, buf);
		if (rc != (int)nblocks) {
			rc = -EIO;
			break;
		} else {
			rc = 0;
		}

		hcfota_send_dump_result(total, bytesread, buf, erasesize);

		offset += erasesize;
		len -= erasesize;
		bytesread += erasesize;
	}

	free(buf);
	hcfota_send_dump_result_done(rc);
}

static int hcfota_program_nand(uint32_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nand");
	int byteswrite = 0;
	off_t startblock;
	size_t nblocks;
	uint32_t erasesize, copied;
	ssize_t rc;
	unsigned char *buf;
	off_t iob_off = 0;
	uint32_t part_length = len;

	if (!mtd) {
		return -ENODEV;
	}

	erasesize = mtd->erasesize;

	buf = malloc(erasesize);
	if (!buf)
		return -ENOMEM;

	/* Skip badblocks if there is any */
	while (MTD_BLOCK_ISBAD(mtd, offset / erasesize) && part_length > 0) {
		offset += erasesize;
		part_length -= erasesize;
	}

	while (part_length > 0) {
		if (MTD_BLOCK_ISBAD(mtd, offset / erasesize)) {
			offset += erasesize;
			part_length -= erasesize;
			continue;
		}

		startblock = offset / mtd->erasesize;
		nblocks = 1;

		rc = MTD_ERASE(mtd, startblock, nblocks);
		if (rc != 0) {
			/* New badblock found, retire it forever */
			MTD_BLOCK_MARKBAD(mtd, startblock);
			offset += erasesize;
			part_length -= erasesize;
			continue;
		}

		if (len > 0) {
			copied = min((uint32_t)len, erasesize);
			memset(buf, 0xff, erasesize);
			if (iobread(NULL, buf, copied, iob_off) != copied) {
				free(buf);
				return HCFOTA_ERR_UPGRADE;
			}
			startblock = offset / mtd->writesize;
			nblocks = erasesize / mtd->writesize;
			rc = MTD_BWRITE(mtd, startblock, nblocks, buf);
			if (rc != (int)nblocks) {
				free(buf);
				return HCFOTA_ERR_UPGRADE;
			}

			len -= copied;
			iob_off += copied;
			byteswrite += copied;
		}

		offset += erasesize;
		part_length -= erasesize;
	}

	free(buf);
	return 0;
}

static struct completion mmc_ready;
static int mmc_connect_notify(struct notifier_block *self, unsigned long action, void *dev)
{
	if (action == SDMMC_NOTIFY_CONNECT)
		complete(&mmc_ready);

	return NOTIFY_OK;
}

static struct notifier_block mmc_connect = {
       .notifier_call = mmc_connect_notify,
};

static int hcfota_check_mmc_ready(void)
{
	struct inode *blkdrv = NULL;
	int rc;

	rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	if (rc < 0) {
		init_completion(&mmc_ready);
		sys_register_notify(&mmc_connect);
		if (wait_for_completion_timeout(&mmc_ready, 50000) == 0) {
			/* timeout */
			return -1;
		}

		usleep(50000);
		return 0;
	}

	close_blockdriver(blkdrv);

	return 0;
}

static void hcfota_dump_sdmmc(uint32_t offset, uint32_t len)
{
	struct inode *blkdrv = NULL;
	struct geometry geo = { 0 };
	int rc;
	uint32_t start_block, blocks;
	char *buf;

	rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	if (rc < 0) {
		if (hcfota_check_mmc_ready() < 0)
			return hcfota_send_dump_result_done(-ENODEV);
		rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	}

	if (rc < 0)
		return hcfota_send_dump_result_done(-ENODEV);

	rc = blkdrv->u.i_bops->geometry(blkdrv, &geo);
	if (rc < 0) {
		close_blockdriver(blkdrv);
		return hcfota_send_dump_result_done(-ENODEV);
	}

	buf = malloc(MAX_TRANSFER_SIZE);
	if (buf == NULL) {
		close_blockdriver(blkdrv);
		return hcfota_send_dump_result_done(-ENOMEM);
	}

	uint32_t rd = 0;
	uint32_t remain;
	int segment;
	while (rd < len) {
		remain = len - rd;
		if (remain > MAX_TRANSFER_SIZE) {
			segment = MAX_TRANSFER_SIZE;
		} else {
			segment = remain;
		}

		start_block = offset / geo.geo_sectorsize;
		blocks = segment / geo.geo_sectorsize;
		rc = blkdrv->u.i_bops->read(blkdrv, buf, start_block, blocks);
		if (rc != (int)blocks) {
			rc = -EIO;
			break;
		} else {
			rc = 0;
		}

		hcfota_send_dump_result(len, rd, buf, segment);
		offset += segment;
		rd += segment;
	}

	close_blockdriver(blkdrv);
	hcfota_send_dump_result_done(rc);
	free(buf);
}

static int hcfota_program_sdmmc(uint32_t offset, uint32_t len)
{
	struct inode *blkdrv = NULL;
	struct geometry geo = { 0 };
	int remaining = len;
	blkcnt_t start_sector;
	blksize_t sectorsize;
	unsigned int nsectors, copied;
	int byteswrite = 0;
	int n = 0x10000;
	int rc;
	void *buf;
	uint32_t part_length = len;
	off_t iob_off = 0;

	buf = malloc(n);
	if (!buf) {
		n >>= 1;
		buf = malloc(n);
	}
	if (!buf) {
		return -ENOMEM;
	}

	rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	if (rc < 0) {
		if (hcfota_check_mmc_ready() < 0) {
			free(buf);
			return -ENODEV;
		}
		rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	}
	if (rc < 0) {
		free(buf);
		return -ENODEV;
	}

	rc = blkdrv->u.i_bops->geometry(blkdrv, &geo);
	if (rc < 0) {
		free(buf);
		close_blockdriver(blkdrv);
		return HCFOTA_ERR_UPGRADE;
	}

	sectorsize = geo.geo_sectorsize;

	while (remaining > 0) {
		copied = min(n, remaining);
		if (iobread(NULL, buf, copied, iob_off) != copied) {
			free(buf);
			close_blockdriver(blkdrv);
			return HCFOTA_ERR_UPGRADE;
		}
		start_sector = offset / sectorsize;
		/* up align to sectorsize */
		nsectors = (copied + sectorsize - 1) / sectorsize;
		rc = blkdrv->u.i_bops->write(blkdrv, buf, start_sector, nsectors);
		if (rc != (int)nsectors) {
			free(buf);
			close_blockdriver(blkdrv);
			return HCFOTA_ERR_UPGRADE;
		}
		iob_off += copied;
		offset += copied;
		byteswrite += copied;
		remaining -= copied;
	}

	close_blockdriver(blkdrv);
	free(buf);

	return 0;
}

extern int memtester_main(int argc, char **argv);
static uint32_t memtester_offset = 0;
static uint32_t memtester_len = 0;
static void hcfota_memtester_task(void *pvParameters)
{
	char physaddrbase[16] = { 0 };
	char wantraw[16] = { 0 };

	sprintf(physaddrbase, "0x%08lx", memtester_offset);
	sprintf(wantraw, "%ldB", memtester_len);
	extern int optind;
	extern int opterr;
	opterr = 0;
	optind = 0;
	memtester_main(4, ((char *[]){ "memtester", "-p", physaddrbase, wantraw, NULL}));
	vTaskDelete(NULL);
}

static void hcfota_sendlog(char *buf, uint32_t len)
{
	d2h_send_log_arg_t *arg = (d2h_send_log_arg_t *)0x80000100;

	memset((void *)arg, 0, 512);
	memcpy((void *)arg->log, buf, len);
	arg->len = (uint32_t)len;
	arg->rpc.dir = UPG_RPC_D2H;
	arg->rpc.id = UPG_RPC_SEND_LOG;
	arg->rpc.status = UPG_RPC_STATUS_SETUP;
	while (arg->rpc.status != UPG_RPC_STATUS_DONE)
		usleep(1000);
}

static int hcfota_memtester(uint32_t offset, uint32_t len)
{
	int fd, ret;
	struct pollfd pfd[1];
	static char buf[512];

	memtester_offset = offset;
	memtester_len = len;
	xTaskCreate(hcfota_memtester_task, (const char *)"memtester",
			    configTASK_STACK_DEPTH, NULL,
			    portPRI_TASK_NORMAL, NULL);

	fd = open("/dev/virtuart_proxy", O_RDWR, 0);

	pfd[0].fd = fd;
	pfd[0].events = POLLIN | POLLRDNORM;

	while (fd >= 0) {
		if (poll(pfd, 1, -1) > 0) {
			if (pfd[0].revents & (POLLRDNORM | POLLIN)) {
				ret = read(fd, buf, 512 - sizeof(d2h_send_log_arg_t) - 1);
				if (ret > 0) {
					hcfota_sendlog(buf, ret);
				}
			}
		}
	}
	return 0;
}

void usb_upgrade_process(void);
void uart_upgrade_process(void);
static void usb_exchange_backend(void *pvParameters)
{
	usb_upgrade_process();
	vTaskDelete(NULL);
}

static void uart_exchange_backend(void *pvParameters)
{
	uart_upgrade_process();
	vTaskDelete(NULL);
}

static void upg_rpc_backend(void *pvParameters)
{
	upg_rpc_arg_t *rpc = (upg_rpc_arg_t *)0x80000000;

	do {
		if (rpc->dir == UPG_RPC_H2D && rpc->status == UPG_RPC_STATUS_SETUP) {
			/* new rpc */
			if (rpc->id == UPG_RPC_REPORT_PROGRESS) {
				h2d_get_progress_arg_t *arg = (h2d_get_progress_arg_t *)rpc;
				arg->progress = upg_progress;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
			} else if (rpc->id == UPG_RPC_REPORT_RESULT) {
				h2d_get_result_arg_t *arg = (h2d_get_result_arg_t *)rpc;
				arg->result = upg_result;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
			} else if (rpc->id == UPG_RPC_DUMP_EFUSE) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_efuse();
			} else if (rpc->id == UPG_RPC_DUMP_OTP_NOR) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_otp_nor();
			} else if (rpc->id == UPG_RPC_DUMP_OTP_SPINAND) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_otp_spinand();
			} else if (rpc->id == UPG_RPC_DUMP_NOR) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t rd_offset = arg->offset;
				uint32_t rd_len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_nor(rd_offset, rd_len);
			} else if (rpc->id == UPG_RPC_DUMP_NAND) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t rd_offset = arg->offset;
				uint32_t rd_len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_nand(rd_offset, rd_len);
			} else if (rpc->id == UPG_RPC_DUMP_SDMMC) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t rd_offset = arg->offset;
				uint32_t rd_len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_dump_sdmmc(rd_offset, rd_len);
			} else if (rpc->id == UPG_RPC_PROGRAM_NOR) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t offset = arg->offset;
				uint32_t len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				int rc = hcfota_program_nor(offset, len);
				hcfota_send_dump_result_done(rc);
			} else if (rpc->id == UPG_RPC_PROGRAM_NAND) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t offset = arg->offset;
				uint32_t len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				int rc = hcfota_program_nand(offset, len);
				hcfota_send_dump_result_done(rc);
			} else if (rpc->id == UPG_RPC_PROGRAM_SDMMC) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t offset = arg->offset;
				uint32_t len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				int rc = hcfota_program_sdmmc(offset, len);
				hcfota_send_dump_result_done(rc);
			} else if (rpc->id == UPG_RPC_TEST_MEMTESTER) {
				h2d_dump_arg_t *arg = (h2d_dump_arg_t *)rpc;
				off_t offset = arg->offset;
				uint32_t len = arg->size;
				arg->rpc.status = UPG_RPC_STATUS_DONE;
				hcfota_memtester(offset, len);
			}
		} else {
			usleep(2000);
		}
	} while (1);

	vTaskDelete(NULL);
}

static void app_main(void *pvParameters)
{
	int ret = 0;

	if (module_init("all") != 0) {
		upg_printf("Updater module init failed\n");
		for (;;);
	}

	upg_printf("HCFOTA bin found at 0x%08lx, bin size %ld\n", (uint32_t)pFota, FotaSize);
	if (ConnectType == CONNECTTYPE_UART || ConnectType == CONNECTTYPE_USB) {
		if (FotaPayloadMagic == FOTA_PAYLOAD_MAGIC)
			ret = hcfota_memory((const char *)pFota, FotaSize, hcfota_report, 0);
		else if (FotaPayloadMagic == FOTA_DUMP_MAGIC) {
			upg_printf("HCFOTA dump on-going!\n");
			while (1)
				usleep(100000);
		}
		else
			ret = hcfota_memory2_nocheck(hcfota_request_read, hcfota_report, 0);
	} else {
		ret = hcfota_memory((const char *)pFota, FotaSize, hcfota_report, 0);
	}

	if (ret) {
		upg_printf("Upgrade failed\n");
	} else {
		upg_printf("Upgrade success\n");
	}

	upg_result = (uint32_t)ret;

	for (;;)
		usleep(10000);

	vTaskDelete(NULL);
}
