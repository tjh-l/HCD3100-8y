#include <generated/br2_autoconf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>
#include <cpu_func.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/math.h>
#include <linux/minmax.h>
#include <asm-generic/div64.h>
#include <linux/mtd/mtd.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/fs/fs.h>
#include <kernel/lib/crc32.h>
#include <kernel/io.h>
#include <kernel/completion.h>
#include "hcfota.h"

#define CONNECTTYPE_UART 0xa5a55a5a
#define CONNECTTYPE_USB  0xaaaa5555
int __attribute__((weak)) soc_printf(const char *format, ...)
{
}

#define hcfota_printf(...)                                                     \
	do {                                                                   \
		printf(__VA_ARGS__);                                           \
		if (*((unsigned long *)0x800001F8) != CONNECTTYPE_UART &&      \
		    *((unsigned long *)0x800001F8) != CONNECTTYPE_USB) {       \
			soc_printf(__VA_ARGS__);                               \
		}                                                              \
	} while (0)

/*
 * Norflash: read/write device address and length can be random
 * Nandflash/Spi-Nandflash : read/write device address and length must be aligned with erasesize
 * eMMC/SD-Card : read/write device address and length must be aligned with geo_sectorsize
 */

enum {
	IH_COMP_NONE = 0, /*  No   Compression Used       */
	IH_COMP_GZIP, /* gzip  Compression Used       */
	IH_COMP_LZMA, /* lzma  Compression Used       */
};

enum {
	IH_DEVT_SPINOR = 0, /*  spi norflash       */
	IH_DEVT_SPINAND, /* spi nandflash       */
	IH_DEVT_NAND, /* nandflash       */
	IH_DEVT_EMMC, /* emmc or sd-card       */
};

enum {
	IH_ENTTRY_NORMAL = 0, /*  normal entry       */
	IH_ENTTRY_REMAP = 1, /*  remap entry       */
};

typedef struct table_entry {
	uint8_t comp;
	char *sname; /* short (input) name to find table entry */
} table_entry_t;

struct hcfota_progress {
	uint32_t step;
	uint32_t next;
	uint32_t current;
	uint32_t total;
	uint32_t finished;
	hcfota_report_t report_cb;
	unsigned long usrdata;
};

struct hcfota_iobuffer {
	FILE *fp;
	void *fota;
	unsigned long fota_size;
	hcfota_request_read_t read_cb;
	unsigned long usrdata;
	size_t size;   /* valid data length in current buffer */
	size_t len;   /* valid data length in current buffer */
	size_t pos;   /* start position inside current buffer */
	off_t offset; /* offset in original file or buffer */
	char buf[0];
};

static const table_entry_t hcfota_comp[] = {
	{ IH_COMP_NONE, "none" },
	{ IH_COMP_GZIP, "gzip" },
	{ IH_COMP_LZMA, "lzma" },
	{ -1, "" },
};

static const table_entry_t hcfota_devtype[] = {
	{ IH_DEVT_SPINOR, "spinor" },
	{ IH_DEVT_SPINAND, "spinand" },
	{ IH_DEVT_NAND, "nand" },
	{ IH_DEVT_EMMC, "emmc" },
	{ -1, "" },
};

static struct hcfota_iobuffer *iobopen_memory(const char *buf, unsigned long size, int iobuffer_sz)
{
	struct hcfota_iobuffer* iob;

	iob = (struct hcfota_iobuffer*)malloc(sizeof(*iob) + iobuffer_sz);
	if (!iob) {
		return NULL;
	}
	memset(iob, 0, sizeof(*iob));

	iob->fota = (void *)buf;
	iob->fota_size = size;
	iob->size = iobuffer_sz;

	return iob;
}

static struct hcfota_iobuffer *iobopen_memory2(hcfota_request_read_t read_cb, unsigned long usrdata, int iobuffer_sz)
{
	struct hcfota_iobuffer* iob;

	iob = (struct hcfota_iobuffer*)malloc(sizeof(*iob) + iobuffer_sz);
	if (!iob) {
		return NULL;
	}
	memset(iob, 0, sizeof(*iob));

	iob->read_cb = read_cb;
	iob->usrdata = usrdata;
	iob->size = iobuffer_sz;

	return iob;
}

static struct hcfota_iobuffer *iobopen_url(const char *filename, int iobuffer_sz)
{
	struct hcfota_iobuffer* iob;
	FILE* fp;

	fp = fopen(filename, "rb");
	if (!fp) {
		return NULL;
	}

	iob = (struct hcfota_iobuffer*)malloc(sizeof(*iob) + iobuffer_sz);
	if (!iob) {
		fclose(fp);
		return NULL;
	}
	memset(iob, 0, sizeof(*iob));

	iob->fp = fp;
	iob->size = iobuffer_sz;

	fseek(iob->fp, iob->offset, SEEK_SET);

	return iob;
}

static int iobclose(struct hcfota_iobuffer *iob)
{
	if (iob) {
		if (iob->fp) {
			fclose(iob->fp);
		}
		free(iob);
		return 0;
	}
	return EOF;
}

static size_t iobread(struct hcfota_iobuffer *iob, void* ptr, size_t size, off_t offset)
{
	size_t remain = size;
	size_t nread = 0;
	size_t n;

	if (size == 0)
		return 0;

	if (offset < iob->offset || offset >= (iob->offset + iob->len)) {
		iob->len = 0;
		iob->pos = 0;
		iob->offset = offset;
	}

	iob->pos += (offset - iob->offset);
	iob->offset = offset;

	while (remain > 0) {
		if (iob->pos >= iob->len) {
			if (iob->fp) {
				fseek(iob->fp, iob->offset, SEEK_SET);
				iob->len = fread(iob->buf, 1, iob->size, iob->fp);
			} else if (iob->fota) {
				if ((iob->fota_size - iob->offset) >= iob->size)
					iob->len = iob->size;
				else
					iob->len = iob->fota_size - iob->offset;

				if (iob->len > 0)
					memcpy(iob->buf, iob->fota + iob->offset, iob->len);
			} else if (iob->read_cb) {
				iob->len = iob->read_cb(iob->offset, iob->buf, iob->size, iob->usrdata);
			} else {
				iob->len = 0;
			}

			iob->pos = 0;
			if (iob->len == 0) {
				break;
			}
		}

		n = min(remain, iob->len - iob->pos);
		memcpy((char*)ptr + nread, iob->buf + iob->pos, n);
		nread += n;
		iob->pos += n;
		iob->offset += n;
		remain -= n;
	}

	return nread;
}

static bool url_is_network(const char *url)
{
	if (!strncmp(url, "http://", 7) || !strncmp(url, "https://", 8) || !strncmp(url, "ftp://", 6))
		return true;
	return false;
}

static void hcfota_updata_progress(struct hcfota_progress *progress, enum HCFOTA_REPORT_EVENT event)
{
	int percent;

	if (!progress) {
		return;
	}

	if (!progress->total) {
		percent = 100;
		progress->report_cb(event, percent, progress->usrdata);
		return;
	}

	if ((progress->current < progress->next) && !progress->finished)
		return;

	percent = (int)((long long)progress->current * 100 / progress->total);
	if (percent == 100 && !progress->finished)
		percent = 99;
	progress->report_cb(event, percent, progress->usrdata);
	progress->next = progress->current + progress->step;

	return;
}

static void hcfota_progress_setup(struct hcfota_progress *progress, hcfota_report_t report_cb, unsigned long usrdata, uint32_t step)
{
	memset(progress, 0, sizeof(*progress));
	progress->report_cb = report_cb;
	progress->usrdata = usrdata;
	progress->step =
	progress->next = step;
}

static int hcfota_set_sysdata_version(uint32_t version)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		hcfota_printf("open /dev/persistentmem failed\n");
		return -1;
	}

	sysdata.firmware_version = version;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, firmware_version);
	node.size = sizeof(sysdata.firmware_version);
	node.buf = &sysdata.firmware_version;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		hcfota_printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static int hcfota_persistentmem_mark_invalid(void)
{
	int fd;

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		hcfota_printf("open /dev/persistentmem failed\n");
		return -1;
	}

	ioctl(fd, PERSISTENTMEM_IOCTL_MARK_INVALID, 0);

	close(fd);

	return 0;
}

static void hcfota_set_crc(struct hcfota_header *header, uint32_t crc)
{
	header->crc = crc;
}

static uint32_t hcfota_get_crc(struct hcfota_header *header)
{
	return header->crc;
}

static void hcfota_set_payload_crc(struct hcfota_payload_header *header, uint32_t crc)
{
	header->crc = crc;
}

static uint32_t hcfota_get_payload_crc(struct hcfota_payload_header *header)
{
	return header->crc;
}

static char *hcfota_get_comp_name(uint8_t comp)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(hcfota_comp); i++) {
		if (hcfota_comp[i].comp == comp)
			break;
	}

	return hcfota_comp[i].sname;
}

static char *hcfota_get_devtype_name(uint8_t comp)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(hcfota_devtype); i++) {
		if (hcfota_devtype[i].comp == comp)
			break;
	}

	return hcfota_devtype[i].sname;
}

static void hcfota_show_header(struct hcfota_header *header)
{
	hcfota_printf("HCFOTA HEADER INFO:\n");
	hcfota_printf("    crc:                       0x%08lx\n", header->crc);
	hcfota_printf("    compress type:             %s\n", hcfota_get_comp_name(header->compress_type));
	hcfota_printf("    ignore version check:      %s\n", header->ignore_version_check ? "true" : "false");
	hcfota_printf("    version:                   %lu\n", header->version);
	hcfota_printf("    uncompressed length:       %ld\n", header->uncompressed_length);
	hcfota_printf("    length:                    %ld\n", header->uncompressed_length);
	hcfota_printf("    board:                     %s\n", header->board);
}

static void hcfota_show_payload_header(struct hcfota_payload_header *header)
{
	hcfota_printf("HCFOTA PAYLOAD INFO:\n");
	hcfota_printf("    crc:                       0x%08lx\n", header->crc);
	hcfota_printf("    entry number:              %ld\n", header->entry_number);
}

static void hcfota_show_normal_entry(union hcfota_entry *entry)
{
	hcfota_printf("  new normal entry:\n");
	hcfota_printf("    index:                     %d\n", entry->upgrade.index);
	hcfota_printf("    entry type:                %d\n", entry->upgrade.entry_type);
	hcfota_printf("    upgrade enable:            %s\n", entry->upgrade.upgrade_enable ? "true" : "false");
	hcfota_printf("    device index:              %d\n", entry->upgrade.dev_index);
	hcfota_printf("    device type:               %s\n", hcfota_get_devtype_name(entry->upgrade.dev_type));
	hcfota_printf("    offset in payload:         0x%08lx\n", entry->upgrade.offset_in_payload);
	hcfota_printf("    length:                    0x%08lx\n", entry->upgrade.length);
	hcfota_printf("    erase length:              0x%08lx\n", entry->upgrade.erase_length);
	hcfota_printf("    offset in device:          0x%08lx\n", entry->upgrade.offset_in_dev);
	hcfota_printf("    offset in mtdblock device: 0x%08lx\n", entry->upgrade.offset_in_blkdev);
}

static void hcfota_show_backup_entry(union hcfota_entry *entry)
{
	hcfota_printf("  new backup entry:\n");
	hcfota_printf("    index:                     %d\n", entry->backup.index);
	hcfota_printf("    entry type:                %d\n", entry->backup.entry_type);
	hcfota_printf("    upgrade enable:            %s\n", entry->backup.upgrade_enable ? "true" : "false");
	hcfota_printf("    old device index:          %d\n", entry->backup.old_dev_index);
	hcfota_printf("    old device type:           %s\n", hcfota_get_devtype_name(entry->backup.old_dev_type));
	hcfota_printf("    old offset in device:      0x%08lx\n", entry->backup.old_offset_in_dev);
	hcfota_printf("    old length:                0x%08lx\n", entry->backup.old_length);
	hcfota_printf("    new device index:          %d\n", entry->backup.new_dev_index);
	hcfota_printf("    new device type:           %s\n", hcfota_get_devtype_name(entry->backup.new_dev_type));
	hcfota_printf("    new offset in device:      0x%08lx\n", entry->backup.new_offset_in_dev);
	hcfota_printf("    new length:                0x%08lx\n", entry->backup.new_length);
}

static int hcfota_check(struct hcfota_iobuffer *iob, struct hcfota_header *header)
{
	off_t pos = 0;
	uint32_t crc, crc2;
	int32_t payload_len;
	ssize_t segment = 0x10000;
	struct sysdata sysdata = { 0 };
	void *buf;

	buf = malloc(segment);
	if (!buf)
		return -ENOMEM;

	crc = hcfota_get_crc(header);
	hcfota_set_crc(header, 0);

	pos = sizeof(struct hcfota_header);
	payload_len = header->uncompressed_length;

	crc2 = crc32(0, (const uint8_t *)header, sizeof(struct hcfota_header));
	while (payload_len > 0) {
		if (payload_len < segment) {
			segment = payload_len;
		}

		if (iobread(iob, buf, segment, pos) != (size_t)segment)
			break;

		crc2 = crc32(crc2, (const uint8_t *)buf, segment);
		pos += segment;
		payload_len -= segment;
	}

	if (crc2 != crc) {
		hcfota_printf("crc check failed!\n");
		hcfota_printf("crc calc 	0x%08lx\n", crc2);
		hcfota_printf("crc in fota 	0x%08lx\n", crc);
		free(buf);
		return HCFOTA_ERR_HEADER_CRC;
	} else {
		hcfota_printf("crc check success!\n");
	}

	if (!sys_get_sysdata(&sysdata)) {
		hcfota_printf("version check: current %lu, new %lu\n", sysdata.firmware_version, header->version);
		//if (header->version <= sysdata.firmware_version && !header->ignore_version_check) 
		if (header->version == sysdata.firmware_version) //zhp 241101
		{
			free(buf);
			return HCFOTA_ERR_VERSION;
		}
	}

	if (header->ignore_version_check)
		hcfota_printf("version check is ignored\n");
	else
		hcfota_printf("version check is NOT ignored\n");

	if (header->ignore_version_update)
		hcfota_printf("system check is ignored\n");
	else
		hcfota_printf("system check is NOT ignored\n");
	hcfota_printf("\n");

	free(buf);
	return 0;
}

static int hcfota_info_internal(struct hcfota_iobuffer *iob)
{
	struct hcfota_header header;
	struct hcfota_payload_header ph;
	int rc = 0;
	unsigned int i;

	rc = iobread(iob, &header, sizeof(header), 0);
	if (rc != sizeof(header)) {
		return HCFOTA_ERR_LOADFOTA;
	}

	rc = hcfota_check(iob, &header);
	if (rc) {
		return rc;
	}

	hcfota_show_header(&header);

	rc = iobread(iob, &ph, sizeof(ph), sizeof(struct hcfota_header));
	if (rc != sizeof(ph)) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_show_payload_header(&ph);

	for (i = 0; i < ph.entry_number; i++) {
		if (ph.entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			hcfota_show_normal_entry(&ph.entry[i]);
		} else if (ph.entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			hcfota_show_backup_entry(&ph.entry[i]);
		}
	}

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

int hcfota_info_memory(const char *buf, unsigned long size)
{
	struct hcfota_iobuffer *iob;
	int rc;

	iob = iobopen_memory(buf, size, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_printf("Show info from %p\n", buf);

	rc = hcfota_info_internal(iob);

	iobclose(iob);

	return rc;
}

int hcfota_info_memory2(hcfota_request_read_t read_cb, unsigned long usrdata)
{
	struct hcfota_iobuffer *iob;
	int rc;

	iob = iobopen_memory2(read_cb, usrdata, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	rc = hcfota_info_internal(iob);

	iobclose(iob);

	return rc;
}

int hcfota_info_url(const char *url)
{
	struct hcfota_iobuffer *iob;
	int rc;

	if (url_is_network(url)) {
		rc = hcfota_download(url, "/tmp/hcfota.bin", NULL, 0);
		if (rc)
			return rc;
		return hcfota_info_url("/tmp/hcfota.bin");
	}

	hcfota_printf("Show info from %s\n", url);

	iob = iobopen_url(url, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	rc = hcfota_info_internal(iob);

	iobclose(iob);

	return rc;
}

int hcfota_download(const char *url, const char *path, hcfota_report_t report_cb, unsigned long usrdata)
{
	hcfota_printf("Downloading from %s to %s\n", url, path);
	return -1;
}

int hcfota_reboot(unsigned long modes)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	hcfota_printf("Reboot mode %ld\n", modes);
	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		hcfota_printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		new_node.size = sizeof(struct sysdata);
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			hcfota_printf("get sysdata failed\n");
			close(fd);
			return -1;
		}
	}

	if (sysdata.ota_detect_modes != modes) {
		sysdata.ota_detect_modes = modes;
		node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		node.offset = offsetof(struct sysdata, ota_detect_modes);
		node.size = sizeof(sysdata.ota_detect_modes);
		node.buf = &sysdata.ota_detect_modes;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
			hcfota_printf("put sysdata failed\n");
			close(fd);
			return -1;
		}
	}

	close(fd);
	reset();

	return 0;
}

static int hcfota_set_doing(uint8_t doing)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		hcfota_printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		new_node.size = sizeof(struct sysdata);
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			hcfota_printf("get sysdata failed\n");
			close(fd);
			return -1;
		}
	}

	sysdata.ota_doing = !!doing;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, ota_doing);
	node.size = sizeof(sysdata.ota_doing);
	node.buf = &sysdata.ota_doing;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		hcfota_printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static int hcfota_good_block_space(uint8_t dev_type, off_t off, uint32_t len)
{
	uint32_t toff = (uint32_t)off;
	struct mtd_dev_s *mtd = NULL;
	uint32_t bad_len = 0;
	uint32_t remaining = len;
	int rc;

	/* spi-nor has no bad blocks */
	if (dev_type == IH_DEVT_SPINOR)
		return len;
	/* emmc has no bad blocks, off & len must be aligned with geo_sectorsize */
	else if (dev_type == IH_DEVT_EMMC) {
		struct inode *blkdrv = NULL;
		struct geometry geo = { 0 };

		rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
		if (rc < 0) {
			if (hcfota_check_mmc_ready() < 0)
				return 0;
			rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
		}

		if (rc < 0) {
			hcfota_printf("/dev/mmcblk0 not found!\n");
			return 0;
		}

		rc = blkdrv->u.i_bops->geometry(blkdrv, &geo);
		close_blockdriver(blkdrv);
		if (rc < 0) {
			return 0;
		}
		if (toff & (geo.geo_sectorsize - 1))
			return 0;
		if (len & (geo.geo_sectorsize - 1))
			return 0;
		return len;
	}
	/* other device types are not supported yet */
	else if (dev_type == IH_DEVT_SPINAND || dev_type == IH_DEVT_NAND) {
		mtd = get_mtd_device_nm("mtd_nand");
		if (!mtd) {
			hcfota_printf("nand mtd block device not found!\n");
			return 0;
		}

		/* The nand device needs the offset and length be aligned with erasesize */
		if (toff & (mtd->erasesize - 1))
			return 0;

		if (len & (mtd->erasesize - 1))
			return 0;

		while (remaining > 0) {
			if (MTD_BLOCK_ISBAD(mtd, off / mtd->erasesize)) {
				bad_len += mtd->erasesize;
			}

			off += mtd->erasesize;
			remaining -= mtd->erasesize;
		}

		if (len > bad_len)
			return (len - bad_len);

		return 0;
	}

	return 0;
}

static int hcfota_check_block_space(union hcfota_entry *entry)
{
	int rc = HCFOTA_ERR_UPGRADE;

	if (!entry->backup.upgrade_enable)
		return 0;

	if (entry->upgrade.entry_type == IH_ENTTRY_NORMAL) {
		off_t offset = (off_t)entry->upgrade.offset_in_dev;
		uint32_t file_len = entry->upgrade.length;
		uint32_t partition_len = entry->upgrade.erase_length;
		partition_len = hcfota_good_block_space(entry->upgrade.dev_type, offset, partition_len);
		if (partition_len < file_len) {
			hcfota_printf(
				"file size %ld(0x%lx) is bigger than valid partition size %ld(0x%lx) (without badblocks), original size is %ld(0x%lx)!\n",
				file_len, file_len, partition_len, partition_len, entry->upgrade.erase_length, entry->upgrade.erase_length);
			rc = HCFOTA_ERR_UPGRADE;
		} else
			rc = 0;
	} else if (entry->upgrade.entry_type == IH_ENTTRY_REMAP) {
		off_t old_offset = (off_t)entry->backup.old_offset_in_dev;
		off_t new_offset = (off_t)entry->backup.new_offset_in_dev;
		uint32_t old_len = entry->backup.old_length;
		uint32_t new_len = entry->backup.new_length;
		old_len = hcfota_good_block_space(entry->backup.old_dev_type, old_offset, old_len);
		new_len = hcfota_good_block_space(entry->backup.new_dev_type, new_offset, new_len);
		if (new_len < old_len) {
			hcfota_printf(
				"new valid partition size %ld(0x%lx) (without badblocks) is smaller than old valid paritition size %ld(0x%lx) (without badblocks)!\n",
				new_len, new_len, old_len, old_len);
			rc = HCFOTA_ERR_UPGRADE;
		} else
			rc = 0;
	}

	return rc;
}

/*
 * 1. offset might not be aligned with erasesize
 * 2. len might not be aligned with erasesize
 * 3. there IS NOT badblocks in nor partition
 */
static int hcfota_read_mtd_nor(void *buf, off_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nor");
	ssize_t rc;

	if (!mtd)
		return 0;

	rc = MTD_READ(mtd, offset, len, buf);

	return rc == (ssize_t)len ? len : 0;
}

static int hcfota_erase_mtd(const char *device)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm(device);
	off_t startblock;
	size_t nblocks;

	if (!mtd)
		return 0;

	startblock = 0;
	nblocks = mtd->size / mtd->erasesize;
	return MTD_ERASE(mtd, startblock, nblocks);
}

/*
 * Below conditions are checked in hcfota_check_block_space()
 * 1. offset is aligned with erasesize
 * 2. len is aligned with erasesize
 * 3. there might be badblocks in nand partition
 */
static int hcfota_read_mtd_nand(void *buf, off_t offset, uint32_t len)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nand");
	int bytesread = 0;
	off_t startblock;
	size_t nblocks;
	uint32_t erasesize;
	ssize_t rc;

	if (!mtd)
		return 0;

	erasesize = mtd->erasesize;

	/* Skip badblocks if there is any */
	while (MTD_BLOCK_ISBAD(mtd, offset / erasesize) && len > 0) {
		offset += erasesize;
		len -= erasesize;
	}

	while (len > 0) {
		if (MTD_BLOCK_ISBAD(mtd, offset / erasesize)) {
			offset += erasesize;
			len -= erasesize;
			continue;
		}

		startblock = offset / mtd->writesize;
		nblocks = erasesize / mtd->writesize;

		rc = MTD_BREAD(mtd, startblock, nblocks, buf);
		if (rc != (int)nblocks) {
			return 0;
		}

		offset += erasesize;
		buf += erasesize;
		len -= erasesize;
		bytesread += erasesize;
	}

	return bytesread;
}

/*
 * Below conditions are checked in hcfota_check_block_space()
 * 1. offset is aligned with geo_sectorsize
 * 2. len is aligned with geo_sectorsize
 * 3. there IS NOT badblocks in emmc partition
 */
static int hcfota_read_emmc(void *buf, blkcnt_t offset, uint32_t len)
{
	struct inode *blkdrv = NULL;
	struct geometry geo = { 0 };
	int rc;

	rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	if (rc < 0) {
		if (hcfota_check_mmc_ready() < 0)
			return 0;
		rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	}

	if (rc < 0)
		return 0;

	rc = blkdrv->u.i_bops->geometry(blkdrv, &geo);
	if (rc < 0) {
		close_blockdriver(blkdrv);
		return 0;
	}

	offset /= geo.geo_sectorsize;
	len /= geo.geo_sectorsize;

	rc = blkdrv->u.i_bops->read(blkdrv, buf, offset, len);
	if (rc != (int)len) {
		close_blockdriver(blkdrv);
		return 0;
	}

	close_blockdriver(blkdrv);

	return rc * geo.geo_sectorsize;
}

static int hcfota_write_mtd_nor(struct hcfota_iobuffer *iob, off_t iob_off, int len, off_t offset, uint32_t part_length, struct hcfota_progress *progress)
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

	if (!mtd) {
		hcfota_printf("nor mtd block device not found!\n");
		return HCFOTA_ERR_UPGRADE;
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
		return HCFOTA_ERR_UPGRADE;
	buf2 = malloc(n);
	if (!buf2) {
		free(buf);
		return HCFOTA_ERR_UPGRADE;
	}

	mask = n - 1;

	src_remaining = len;
	dst_remaining = part_length;
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

		if (iobread(iob, buf2, copied, iob_off) != copied) {
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

		if (progress) {
			progress->current += copied;
			hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
		}
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

		if (iobread(iob, buf, n, iob_off) != n) {
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

		if (progress) {
			progress->current += n;
			hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
		}
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
		if (iobread(iob, buf, copied, iob_off) != copied) {
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

		if (progress) {
			progress->current += copied;
			hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
		}
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

	return byteswrite;
}

static int hcfota_write_mtd_nand(struct hcfota_iobuffer *iob, off_t iob_off, int len, off_t offset, uint32_t part_length, struct hcfota_progress *progress)
{
	struct mtd_dev_s *mtd = get_mtd_device_nm("mtd_nand");
	int byteswrite = 0;
	off_t startblock;
	size_t nblocks;
	uint32_t erasesize, copied;
	ssize_t rc;
	unsigned char *buf;

	if (!mtd) {
		hcfota_printf("nand mtd block device not found!\n");
		return HCFOTA_ERR_UPGRADE;
	}

	erasesize = mtd->erasesize;

	buf = malloc(erasesize);
	if (!buf)
		return HCFOTA_ERR_UPGRADE;

	/* Skip badblocks if there is any */
	while (MTD_BLOCK_ISBAD(mtd, offset / erasesize) && part_length > 0) {
		hcfota_printf("skip bad block, offset %llu\r\n", offset);
		offset += erasesize;
		part_length -= erasesize;
	}

	while (part_length > 0) {
		if (MTD_BLOCK_ISBAD(mtd, offset / erasesize)) {
			hcfota_printf("skip bad block, offset %llu\r\n", offset);
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
			if (iobread(iob, buf, copied, iob_off) != copied) {
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

			if (progress) {
				progress->current += copied;
				hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
			}
		}

		offset += erasesize;
		part_length -= erasesize;
	}

	free(buf);
	return byteswrite;
}

static int hcfota_write_emmc(struct hcfota_iobuffer *iob, off_t iob_off, int len, blkcnt_t offset, uint32_t part_length, struct hcfota_progress *progress)
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
	
	if ((int)part_length < len)
		return HCFOTA_ERR_UPGRADE;
	
	buf = malloc(n);
	if (!buf) {
		n >>= 1;
		buf = malloc(n);
	}
	if (!buf) {
		return HCFOTA_ERR_UPGRADE;
	}

	rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	if (rc < 0) {
		if (hcfota_check_mmc_ready() < 0) {
			free(buf);
			return HCFOTA_ERR_UPGRADE;
		}
		rc = open_blockdriver("/dev/mmcblk0", MS_RDONLY, &blkdrv);
	}
	if (rc < 0) {
		hcfota_printf("/dev/mmcblk0 device not found!\n");
		free(buf);
		return HCFOTA_ERR_UPGRADE;
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
		if (iobread(iob, buf, copied, iob_off) != copied) {
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

		if (progress) {
			progress->current += copied;
			hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
		}
	}

	close_blockdriver(blkdrv);
	free(buf);

	return byteswrite;
}

static int hcfota_do_backup(union hcfota_entry *entry, struct hcfota_progress *progress)
{
	void *buf = NULL;
	int len = 0;
	int rc;

	if (!entry->backup.upgrade_enable)
		return 0;

	buf = malloc(entry->backup.old_length);
	if (!buf)
		return -ENOMEM;

	if (entry->backup.old_dev_type == IH_DEVT_SPINOR) {
		off_t offset = (off_t)entry->backup.old_offset_in_dev;
		uint32_t old_length = entry->backup.old_length;
		len = hcfota_read_mtd_nor(buf, offset, old_length);
	} else if (entry->backup.old_dev_type == IH_DEVT_SPINAND ||
		   entry->backup.old_dev_type == IH_DEVT_NAND) {
		off_t offset = (off_t)entry->backup.old_offset_in_dev;
		uint32_t old_length = entry->backup.old_length;
		len = hcfota_read_mtd_nand(buf, offset, old_length);
	} else if (entry->backup.old_dev_type == IH_DEVT_EMMC) {
		blkcnt_t offset = (blkcnt_t)entry->backup.old_offset_in_dev;
		uint32_t old_length = entry->backup.old_length;
		len = hcfota_read_emmc(buf, offset, old_length);
	}

	if (len <= 0) {
		free(buf);
		return HCFOTA_ERR_UPGRADE;
	}

	struct hcfota_iobuffer *iob;
	iob = iobopen_memory(buf, len, 0x1000);
	if (!iob) {
		free(buf);
		return HCFOTA_ERR_UPGRADE;
	}

	rc = HCFOTA_ERR_UPGRADE;
	if (entry->backup.new_dev_type == IH_DEVT_SPINOR) {
		off_t offset = (off_t)entry->backup.new_offset_in_dev;
		uint32_t limit = entry->backup.new_length;
		rc = hcfota_write_mtd_nor(iob, 0, len, offset, limit, NULL);
	} else if (entry->backup.old_dev_type == IH_DEVT_SPINAND ||
		   entry->backup.old_dev_type == IH_DEVT_NAND) {
		off_t offset = (off_t)entry->backup.new_offset_in_dev;
		uint32_t limit = entry->backup.new_length;
		rc = hcfota_write_mtd_nand(iob, 0, len, offset, limit, NULL);
	} else if (entry->backup.new_dev_type == IH_DEVT_EMMC) {
		blkcnt_t offset = (blkcnt_t)entry->backup.new_offset_in_dev;
		uint32_t limit = entry->backup.new_length;
		rc = hcfota_write_emmc(iob, 0, len, offset, limit, NULL);
	}

	free(buf);
	iobclose(iob);
	if (rc == len) {
		rc = 0;
		progress->current += min(entry->backup.old_length, entry->backup.new_length);
		hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
	} else {
		rc = HCFOTA_ERR_UPGRADE;
	}

	return rc;
}

static int hcfota_do_upgrade(struct hcfota_iobuffer *iob, union hcfota_entry *entry, struct hcfota_progress *progress)
{
	off_t iob_off;
	off_t offset;
	int len;
	uint32_t limit;
	int rc;

	if (!entry->upgrade.upgrade_enable)
		return 0;

	rc = HCFOTA_ERR_UPGRADE;
	if (entry->upgrade.dev_type == IH_DEVT_SPINOR) {
		iob_off = entry->upgrade.offset_in_payload;
		offset = entry->upgrade.offset_in_dev;
		len = entry->upgrade.length;
		limit = entry->upgrade.erase_length;
		iob_off += sizeof(struct hcfota_header);
		iob_off += sizeof(struct hcfota_payload_header);
		rc = hcfota_write_mtd_nor(iob, iob_off, len, offset, limit, progress);
		if (rc == len)
			rc = 0;
	} else if (entry->upgrade.dev_type == IH_DEVT_SPINAND ||
		   entry->upgrade.dev_type == IH_DEVT_NAND) {
		iob_off = entry->upgrade.offset_in_payload;
		offset = entry->upgrade.offset_in_dev;
		len = entry->upgrade.length;
		limit = entry->upgrade.erase_length;
		iob_off += sizeof(struct hcfota_header);
		iob_off += sizeof(struct hcfota_payload_header);
		rc = hcfota_write_mtd_nand(iob, iob_off, len, offset, limit, progress);
		if (rc == len)
			rc = 0;
	} else if (entry->upgrade.dev_type == IH_DEVT_EMMC) {
		iob_off = entry->upgrade.offset_in_payload;
		offset = entry->upgrade.offset_in_dev;
		len = entry->upgrade.length;
		limit = entry->upgrade.erase_length;
		iob_off += sizeof(struct hcfota_header);
		iob_off += sizeof(struct hcfota_payload_header);
		rc = hcfota_write_emmc(iob, iob_off, len, offset, limit, progress);
		if (rc == len)
			rc = 0;
	} else {
		rc = HCFOTA_ERR_UPGRADE;
	}

	return rc;
}

static int hcfota_internal(struct hcfota_iobuffer *iob, struct hcfota_progress *progress, int do_check, bool checkonly)
{
	struct hcfota_header header;
	struct hcfota_payload_header ph;
	struct sysdata sysdata = { 0 };
	int rc = 0;
	unsigned int i;

	rc = iobread(iob, &header, sizeof(header), 0);
	if (rc != sizeof(header)) {
		return HCFOTA_ERR_LOADFOTA;
	}

	if (do_check) {
		rc = hcfota_check(iob, &header);
		if (rc) {
			return rc;
		}
	}

	if (!sys_get_sysdata(&sysdata)) {
		//if (header.version <= sysdata.firmware_version &&!header.ignore_version_check) 
		if (header.version == sysdata.firmware_version) //zhp 241101
		{
			hcfota_printf("Version check failed!\n");
			return HCFOTA_ERR_VERSION;
		}
	}

	if (header.compress_type != IH_COMP_NONE) {
		return HCFOTA_ERR_DECOMPRESSS;
	}

	if (checkonly)
		return 0;

	rc = iobread(iob, &ph, sizeof(ph), sizeof(struct hcfota_header));
	if (rc != sizeof(ph)) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_set_doing(1);

	if (header.ignore_version_update)
		hcfota_persistentmem_mark_invalid();

	if (header.erase_nor_chip) {
		hcfota_printf("Erase entire norflash...!\n");
		hcfota_erase_mtd("mtd_nor");
		hcfota_printf("Erase entire norflash done!\n");
	}
	if (header.erase_nand_chip) {
		hcfota_printf("Erase entire nandflash...!\n");
		hcfota_erase_mtd("mtd_nand");
		hcfota_printf("Erase entire nandflash done!\n");
	}

	for (i = 0; i < ph.entry_number; i++) {
		if (hcfota_check_block_space(&ph.entry[i]))
			return HCFOTA_ERR_UPGRADE;

		if (ph.entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			if (!ph.entry[i].upgrade.upgrade_enable)
				continue;
			progress->total += ph.entry[i].upgrade.length;
		} else if (ph.entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			if (!ph.entry[i].backup.upgrade_enable)
				continue;
			progress->total += min(ph.entry[i].backup.old_length,
					       ph.entry[i].backup.new_length);
		}
	}

	hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);

	/* Do backup first */
	rc = 0;
	for (i = 0; i < ph.entry_number; i++) {
		if (ph.entry[i].upgrade.entry_type != IH_ENTTRY_REMAP)
			continue;

		rc |= hcfota_do_backup(&ph.entry[i], progress);
		if (rc) {
			hcfota_printf("Backup partition failed\n");
			hcfota_show_backup_entry(&ph.entry[i]);
		}
	}

	/* Do upgrade */
	for (i = 0; i < ph.entry_number; i++) {
		if (ph.entry[i].upgrade.entry_type != IH_ENTTRY_NORMAL)
			continue;

		rc |= hcfota_do_upgrade(iob, &ph.entry[i], progress);
		if (rc) {
			hcfota_printf("Upgrade partition failed\n");
			hcfota_show_normal_entry(&ph.entry[i]);
		}
	}

	if (rc) {
		hcfota_printf("\nUpgrade fail\n");
	} else {
		progress->finished = 1;
		hcfota_set_doing(0);
		hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);
		if (!header.ignore_version_update)
			hcfota_set_sysdata_version(header.version);
		hcfota_printf("\nUpgrade success\n");
	}

	return rc ? HCFOTA_ERR_UPGRADE : 0;
}

static int __hcfota_url(const char *url, hcfota_report_t report_cb, unsigned long usrdata, bool checkonly)
{
	int rc;
	struct hcfota_iobuffer *iob;
	struct hcfota_progress progress = { 0 };

	hcfota_printf("Upgrade from %s\n", url);

	if (url_is_network(url)) {
		rc = hcfota_download(url, "/tmp/hcfota.bin", report_cb, usrdata);
		if (rc)
			return HCFOTA_ERR_DOWNLOAD;
		return hcfota_url("/tmp/hcfota.bin", report_cb, usrdata);
	}

	iob = iobopen_url(url, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_progress_setup(&progress, report_cb, usrdata, 1);

	rc = hcfota_internal(iob, &progress, 1, checkonly);

	iobclose(iob);

	return rc;
}

int hcfota_url_checkonly(const char *url, unsigned long usrdata)
{
	return __hcfota_url(url, NULL, usrdata, 1);
}

int hcfota_url(const char *url, hcfota_report_t report_cb, unsigned long usrdata)
{
	return __hcfota_url(url, report_cb, usrdata, 0);
}

static int __hcfota_memory(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata, bool checkonly)
{
	struct hcfota_iobuffer *iob;
	struct hcfota_progress progress = { 0 };
	int rc;

	hcfota_printf("Upgrade info from %p\n", buf);

	iob = iobopen_memory(buf, size, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_progress_setup(&progress, report_cb, usrdata, 1);

	rc = hcfota_internal(iob, &progress, 1, checkonly);

	iobclose(iob);

	return rc;
}

int hcfota_memory_checkonly(const char *buf, unsigned long size, unsigned long usrdata)
{
	return __hcfota_memory(buf, size, NULL, usrdata, 1);
}

int hcfota_memory(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata)
{
	return __hcfota_memory(buf, size, report_cb, usrdata, 0);
}

static int __hcfota_memory2(hcfota_request_read_t read_cb, hcfota_report_t report_cb, unsigned long usrdata, int do_check, int iob_size, bool checkonly)
{
	struct hcfota_iobuffer *iob;
	struct hcfota_progress progress = { 0 };
	int rc;

	iob = iobopen_memory2(read_cb, usrdata, 0x8000);
	if (!iob) {
		return HCFOTA_ERR_LOADFOTA;
	}

	hcfota_progress_setup(&progress, report_cb, usrdata, 5);

	rc = hcfota_internal(iob, &progress, do_check, checkonly);

	iobclose(iob);

	return rc;
}

int hcfota_memory2_checkonly(hcfota_request_read_t read_cb, unsigned long usrdata)
{
	return __hcfota_memory2(read_cb, NULL, usrdata, 1, 0x8000, 1);
}

int hcfota_memory2(hcfota_request_read_t read_cb, hcfota_report_t report_cb, unsigned long usrdata)
{
	return __hcfota_memory2(read_cb, report_cb, usrdata, 1, 0x8000, 0);
}

int hcfota_memory2_nocheck(hcfota_request_read_t read_cb, hcfota_report_t report_cb, unsigned long usrdata)
{
	return __hcfota_memory2(read_cb, report_cb, usrdata, 0, 0x10000, 0);
}
