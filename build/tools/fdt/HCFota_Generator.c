#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <hcfota.h>

#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
#include "fdt_api.h"

#include "util.h"

#include "iniparser/src/iniparser.h"

#define max(a, b)                                                              \
	({                                                                     \
		typeof(a) _a = a;                                              \
		typeof(b) _b = b;                                              \
		_a > _b ? _a : _b;                                             \
	})

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
	IH_DEVT_SPINOR1, /*  spi norflash 1      */
	IH_DEVT_NONE, /* no device specified       */
};

enum {
	IH_ENTTRY_NORMAL = 0, /*  normal entry       */
	IH_ENTTRY_REMAP = 1, /*  remap entry       */
	IH_ENTTRY_DDRINFO = 2, /*  ddrinfo entry       */
	IH_ENTTRY_UPDATER = 3, /*  updater entry       */
	IH_ENTTRY_PARTINFO = 4, /*  partition info entry       */
};

static const char *map_entry_type[] = {
	[IH_ENTTRY_NORMAL] = "normal",
	[IH_ENTTRY_REMAP] = "remap",
	[IH_ENTTRY_DDRINFO] = "ddrinfo",
	[IH_ENTTRY_UPDATER] = "updater",
	[IH_ENTTRY_PARTINFO] = "partinfo",
};

static const char *map_device_type[] = {
	[IH_DEVT_SPINOR] = "spinor",
	[IH_DEVT_SPINAND] = "spinand",
	[IH_DEVT_NAND] = "nand",
	[IH_DEVT_EMMC] = "sdmmc",
	[IH_DEVT_SPINOR1] = "spinor1",
	[IH_DEVT_NONE] = "(none)",
};

struct hgpt_header_s
{
	uint32_t offset;
	uint32_t size;
	uint32_t reserved[2];
	char name[16];
};

#define HGPT_MAGIC 0xe00ce00c
struct hgpt_ptable_s
{
	uint32_t magic;
	uint32_t revision;
	uint32_t hgpt_size;
	uint32_t num_partition_entries;
	uint32_t reserved[4];
	struct hgpt_header_s gpt_header[0];
};

extern uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len);
extern int mkgpt(int argc, char *argv[]);
extern unsigned long long memparse(const char *ptr, char **retptr);

static void print_usage(const char *prog)
{
	printf("Usage: %s [-iodeap]\n", prog);
	puts("  -o --output    output file\n"
	     "  -u --upgrade   hcfota for upgrade\n"
	     "  -d --dtb       DTB binary\n"
	     "  -e --erasenor  erase entire nor chip\n"
	     "  -a --erasenand erase entire nand chip\n"
	     "  -r --raminit   ram init file\n"
	     "  -p --updater   updater file\n"
	     "  -s --skip      skip partition name\n"
	     "  -v --version   firmware version\n"
	     "  -P --product   product name\n"
	     "  -h --help      print this help\n");
}

static uint32_t get_filesize(const char *file)
{
	struct stat sb;

	if (!file)
		return 0;

	if (stat(file, &sb) == -1) {
		die("ERROR : can not open file %s\r\n", file);
		return 0;
	}
	return (uint32_t)sb.st_size;
}

struct parse_args {
	const char *dtb_path;
	const char *output;
	int for_upgrade;
	int erase_nor_chip;
	int erase_nand_chip;
	int version_check;
	const char *ddrinit;
	const char *updater;
	const char *skip_mtdname[100];
	int nb_skip_mtdname;
	uint32_t version;
	const char *product;
	const char *part_label_persistentmem;
	int erase_nor1_chip;
};

static int parse_input_args(struct parse_args *pa, int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "output",    1, 0, 'o' },
			{ "upgrade",   0, 0, 'u' },
			{ "dtb",       1, 0, 'd' },
			{ "erasenor",  0, 0, 'e' },
			{ "erasenor1", 0, 0, 'E' },
			{ "erasenand", 0, 0, 'a' },
			{ "versioncheck", 1, 0, 'c' },
			{ "ram",       1, 0, 'r' },
			{ "updater",   1, 0, 'p' },
			{ "skip",      1, 0, 's' },
			{ "version",   1, 0, 'v' },
			{ "product",   1, 0, 'P' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "o:d:eEauhr:p:s:v:P:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			pa->dtb_path = optarg;
			break;
		case 'o':
			pa->output = optarg;
			break;
		case 'u':
			pa->for_upgrade = 1;
			break;
		case 'e':
			pa->erase_nor_chip = 1;
			break;
		case 'E':
			pa->erase_nor1_chip = 1;
			break;
		case 'a':
			pa->erase_nand_chip = 1;
			break;
		case 'c':
			pa->version_check = !!atoi(optarg);
			break;
		case 'r':
			pa->ddrinit = optarg;
			break;
		case 'p':
			pa->updater = optarg;
			break;
		case 's':
			pa->skip_mtdname[pa->nb_skip_mtdname] = optarg;
			printf("%s\r\n", pa->skip_mtdname[pa->nb_skip_mtdname]);
			pa->nb_skip_mtdname++;
			break;
		case 'v':
			pa->version = strtoul(optarg, NULL, 0);
			break;
		case 'P':
			pa->product = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return -1;
		}
	}
	return 0;
}

static void setup_dtb(const char *dtb_path)
{
	size_t len;
	char *dtb;
	dtb = utilfdt_read(dtb_path, &len);
	if (!dtb)
		die("could not read: %s\n", dtb_path);

	fdt_setup(dtb);
}

static int find_persistentmem(struct parse_args *pa)
{
	int np;
	const char *mtdname = NULL;
	np = fdt_get_node_offset_by_path("/hcrtos/persistentmem");
	if (np < 0) {
		fprintf(stderr, "cannot find persistentmem in DTS\n");
		return -1;
	}

	if (fdt_get_property_string_index(np, "mtdname", 0, &mtdname)) {
		fprintf(stderr, "cannot find mtdname property in /hcrtos/persistentmem in DTS\n");
		return -1;
	}
	pa->part_label_persistentmem = mtdname;
	return 0;
}

static void fixup_spinor_enable(struct hcfota_header *h)
{
	int np;
	np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
	if (np < 0) {
		h->spinor_en_cs0 = 0;
	} else {
		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			h->spinor_en_cs0 = 0;
		} else {
			h->spinor_en_cs0 = 1;
		}
	}
}

static void fixup_spinor_enable1(struct hcfota_header *h)
{
	int np;
	np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1");
	if (np < 0) {
		h->spinor_en_cs1 = 0;
	} else {
		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			h->spinor_en_cs1 = 0;
		} else {
			h->spinor_en_cs1 = 1;
		}
	}
}

static void fixup_spinand_enable(struct hcfota_header *h)
{
	int np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
	if (np < 0) {
		h->spinand_en_cs0 = 0;
		h->spinand_en_cs1 = 0;
	} else {
		const char *status = NULL;
		uint32_t reg = 0;
		if (fdt_get_property_u_32_index(np, "reg", 0, &reg) != 0)
			reg = 0;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			h->spinand_en_cs0 = 0;
			h->spinand_en_cs1 = 0;
		} else {
			if (reg == 0) {
				h->spinand_en_cs0 = 1;
				h->spinand_en_cs1 = 0;
			} else if (reg == 1) {
				h->spinand_en_cs0 = 0;
				h->spinand_en_cs1 = 1;
			}
		}
	}
}

static void fixup_nand_enable(struct hcfota_header *h)
{
	int np = fdt_get_node_offset_by_path("/hcrtos/nand");
	if (np < 0) {
		h->nand_en = 0;
	} else {
		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			h->nand_en = 0;
		} else {
			h->nand_en = 1;
		}
	}
}

static void fixup_sdmmc_enable(struct hcfota_header *h)
{
	int np = fdt_get_node_offset_by_path("/hcrtos/mmc");
	if (np < 0) {
		h->emmc_v20_left_en = 0;
		h->emmc_v20_top_en = 0;
		h->emmc_v30_en = 0;
		return;
	}

	const char *status = NULL;
	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	    !strcmp(status, "disabled")) {
		h->emmc_v20_left_en = 0;
		h->emmc_v20_top_en = 0;
		h->emmc_v30_en = 0;
		return;
	}

	uint32_t pinpad = 0;
	uint32_t pinmux = 0;
	if (fdt_get_property_u_32_index(np, "pinmux-active", 0, &pinpad) != 0)
		pinpad = 0;
	if (fdt_get_property_u_32_index(np, "pinmux-active", 1, &pinmux) != 0)
		pinpad = 0;

	if (pinpad >= 96 && pinpad <= 101) {
		/* Top */
		if (pinmux == 4) {
			h->emmc_v20_left_en = 0;
			h->emmc_v20_top_en = 1;
			h->emmc_v30_en = 0;
		} else {
			h->emmc_v20_left_en = 0;
			h->emmc_v20_top_en = 0;
			h->emmc_v30_en = 1;
		}
	} else {
		/* Left */
		h->emmc_v20_left_en = 1;
		h->emmc_v20_top_en = 0;
		h->emmc_v30_en = 0;
	}
}

static const char *get_file_path(struct parse_args *pa, const char *filename)
{
	static char filepath[4096];
	if (!filename)
		return NULL;
	if (!strcasecmp(filename, "null"))
		return NULL;
	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s", pa->dtb_path);
	char *tmp = strrchr(filepath, '/');
	if (tmp == NULL) {
		memset(filepath, 0, sizeof(filepath));
		sprintf(filepath, "%s", filename);
	} else {
		sprintf(tmp, "/%s", filename);
	}
	return filepath;
}

static const char *get_output_path(struct parse_args *pa, const char **path2nd)
{
	static char str[4096];
	static char str2[4096];
	const char *output = pa->output;
	const char *product = pa->product;
	uint32_t version = pa->version;
	const char *pstr2 = NULL;
	int for_upgrade = pa->for_upgrade;

	memset(str, 0, sizeof(str));
	memset(str2, 0, sizeof(str2));
	if (output) {
		sprintf(str, "%s", output);
		sprintf(str2, "%s", output);
		pstr2 = &str2[0];
		char *tmp = strrchr(str2, '.');
		if (for_upgrade)
			sprintf(strrchr(str, '.'), "%s", ".ota.bin");

		if (version & 0xff000000) {
			if (for_upgrade)
				sprintf(tmp, "_%s_%u.ota.bin", product, version);
			else
				sprintf(tmp, "_%s_%u.bin", product, version);
		} else {
			if (for_upgrade)
				sprintf(tmp, "_%s_v%d.%d.%d.ota.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
			else
				sprintf(tmp, "_%s_v%d.%d.%d.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
		}
	} else {
		sprintf(str, "%s", pa->dtb_path);
		char *tmp = strrchr(str, '/');
		if (tmp == NULL) {
			memset(str, 0, sizeof(str));
			if (version & 0xff000000) {
				if (for_upgrade)
					sprintf(str, "HCFOTA_%s_%u.ota.bin", product, version);
				else
					sprintf(str, "HCFOTA_%s_%u.bin", product, version);
			} else {
				if (for_upgrade)
					sprintf(str, "HCFOTA_%s_v%d.%d.%d.ota.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
				else
					sprintf(str, "HCFOTA_%s_v%d.%d.%d.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
			}
		} else {
			if (version & 0xff000000) {
				if (for_upgrade)
					sprintf(tmp, "/HCFOTA_%s_%u.ota.bin", product, version);
				else
					sprintf(tmp, "/HCFOTA_%s_%u.bin", product, version);
			} else {
				if (for_upgrade)
					sprintf(tmp, "/HCFOTA_%s_v%d.%d.%d.ota.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
				else
					sprintf(tmp, "/HCFOTA_%s_v%d.%d.%d.bin", product, version >> 16, (version >> 8) & 0xff, version & 0xff);
			}
		}
	}
	*path2nd = pstr2;
	return str;
}

static void print_entry_info(union hcfota_entry *e, struct hcfota_entry_info *i)
{
	printf("insert entry (%d)  :\r\n", e->upgrade.index);
	printf("\tdevice type      : %s\r\n", map_device_type[e->upgrade.dev_type]);
	printf("\tentry type       : %s\r\n", map_entry_type[e->upgrade.entry_type]);
	printf("\tpartname         : %s\r\n", i->names[e->upgrade.index]);
	printf("\tupgrade enable   : %s\r\n", e->upgrade.upgrade_enable ? "TRUE" : "FALSE");
	printf("\tfilesize         : %d(0x%x)\r\n", e->upgrade.length, e->upgrade.length);
	printf("\tpartition offset : %d(0x%x)\r\n", e->upgrade.offset_in_dev, e->upgrade.offset_in_dev);
	if (e->upgrade.erase_length == (unsigned int)INT_MAX)
	printf("\tpartition size   : (more than INT_MAX)\r\n");
	else
	printf("\tpartition size   : %d(0x%x)\r\n", e->upgrade.erase_length, e->upgrade.erase_length);
}

static void print_args(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\r\n");
}

int main(int argc, char *argv[])
{
	struct hcfota_header h = { 0 };
	struct hcfota_payload_header ph = { 0 };
	union hcfota_entry e = { 0 };
	union hcfota_entry e_persistentmem = { 0 };
	struct hcfota_entry_info entry_info = { 0 };
	struct parse_args pa = { 0 };
	uint32_t entry_number = 0;
	uint32_t offset_in_payload = 0;
	void *payload = NULL;
	const char *filename_persistentmem = NULL;
	const char *partname_persistentmem = NULL;

	print_args(argc, argv);

	if (parse_input_args(&pa, argc, argv) < 0)
		return -1;

	if (!pa.dtb_path) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	setup_dtb(pa.dtb_path);
	if (find_persistentmem(&pa) < 0) {
		printf("Can not find persistentmem partition\r\n");
		return EXIT_FAILURE;
	}

	fixup_spinor_enable(&h);
	fixup_spinor_enable1(&h);
	fixup_spinand_enable(&h);
	fixup_nand_enable(&h);
	fixup_sdmmc_enable(&h);

	const char *output_path2 = NULL;
	const char *output_path = get_output_path(&pa, &output_path2);
	printf("Generate %s\r\n", output_path);
	if (output_path2)
		printf("Generate %s\r\n", output_path2);

	if (pa.ddrinit != NULL && get_filesize(pa.ddrinit) != -1) {
		memset(&e, 0, sizeof(e));
		e.upgrade.length = get_filesize(pa.ddrinit);
		e.upgrade.index = entry_number;
		e.upgrade.dev_index = -1;
		e.upgrade.dev_type = IH_DEVT_NONE;
		e.upgrade.upgrade_enable = 0;
		e.upgrade.offset_in_payload = offset_in_payload;
		e.upgrade.erase_length = 0;
		e.upgrade.entry_type = IH_ENTTRY_DDRINFO;
		e.upgrade.offset_in_dev = -1;
		if (e.upgrade.length > 0) {
			payload = realloc(payload, offset_in_payload + e.upgrade.length);
			FILE *fp = fopen(pa.ddrinit, "rb");
			fseek(fp, 0, SEEK_SET);
			int ret = fread(payload + offset_in_payload, 1, e.upgrade.length, fp);
			if (ret != e.upgrade.length)
				printf("Error read %s!\n", pa.ddrinit);
			fclose(fp);
		}

		snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "ddrinfo");
		ph.entry[entry_number] = e;
		print_entry_info(&e, &entry_info);
		offset_in_payload += e.upgrade.length;
		entry_number++;
	}

	if (pa.updater != NULL && get_filesize(pa.updater) != -1) {
		memset(&e, 0, sizeof(e));
		e.upgrade.length = get_filesize(pa.updater);
		e.upgrade.index = entry_number;
		e.upgrade.dev_index = -1;
		e.upgrade.dev_type = IH_DEVT_NONE;
		e.upgrade.upgrade_enable = 0;
		e.upgrade.offset_in_payload = offset_in_payload;
		e.upgrade.erase_length = 0;
		e.upgrade.entry_type = IH_ENTTRY_UPDATER;
		e.upgrade.offset_in_dev = -1;
		if (e.upgrade.length > 0) {
			payload = realloc(payload, offset_in_payload + e.upgrade.length);
			FILE *fp = fopen(pa.updater, "rb");
			fseek(fp, 0, SEEK_SET);
			int ret = fread(payload + offset_in_payload, 1, e.upgrade.length, fp);
			if (ret != e.upgrade.length)
				printf("Error read %s!\n", pa.updater);
			fclose(fp);
		}

		snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "updater");
		ph.entry[entry_number] = e;
		print_entry_info(&e, &entry_info);
		offset_in_payload += e.upgrade.length;
		entry_number++;
	}

	do {
		int np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
		if (np < 0)
			break;

		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
		u32 npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (unsigned i = 1; i <= npart; i++) {
			u32 offset = 0;
			u32 size = 0;
			u32 part_upgrade = 1;
			char strbuf[512];
			const char *filename = NULL;
			const char *filepath = NULL;
			const char *partname = NULL;
			uint32_t filesize = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &offset);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);

			if (!part_upgrade)
				continue;

			filepath = get_file_path(&pa, filename);
			filesize = get_filesize(filepath);
			if (filesize > size) {
				printf("Error: The size of %s is %d(0x%x), which is bigger than partition size %d(0x%x)!\n", filename, filesize, filesize, size, size);
				printf("Error: Please increase the partitions size or make the file %s smaller!\n", filename);
				exit(1);
			}

			if (partname && !strncmp(partname, pa.part_label_persistentmem, max(strlen(partname), strlen(pa.part_label_persistentmem)))) {
				h.ignore_version_update = 1;
				filename_persistentmem = filename;
				partname_persistentmem = partname;
				memset(&e_persistentmem, 0, sizeof(e_persistentmem));
				e_persistentmem.upgrade.dev_index = -1;
				e_persistentmem.upgrade.dev_type = IH_DEVT_SPINOR;
				e_persistentmem.upgrade.upgrade_enable = 1;
				e_persistentmem.upgrade.erase_length = size;
				e_persistentmem.upgrade.entry_type = IH_ENTTRY_NORMAL;
				e_persistentmem.upgrade.offset_in_dev = offset;
				/* to be appended at the last entry */
				continue;
			}

			memset(&e, 0, sizeof(e));
			e.upgrade.length = filesize;
			e.upgrade.index = entry_number;
			e.upgrade.dev_index = -1;
			e.upgrade.dev_type = IH_DEVT_SPINOR;
			e.upgrade.upgrade_enable = 1;
			e.upgrade.offset_in_payload = offset_in_payload;
			e.upgrade.erase_length = size;
			e.upgrade.entry_type = IH_ENTTRY_NORMAL;
			e.upgrade.offset_in_dev = offset;
			if (filesize > 0) {
				payload = realloc(payload, offset_in_payload + filesize);
				FILE *fp = fopen(filepath, "rb");
				fseek(fp, 0, SEEK_SET);
				int ret = fread(payload + offset_in_payload, 1, filesize, fp);
				if (ret != filesize)
					die("Error read %s!\n", filepath);
				fclose(fp);
			}

			snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
			ph.entry[entry_number] = e;
			print_entry_info(&e, &entry_info);
			offset_in_payload += filesize;
			entry_number++;
		}
	} while (0);

	do {
		int np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1");
		if (np < 0)
			break;

		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1/partitions");
		u32 npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (unsigned i = 1; i <= npart; i++) {
			u32 offset = 0;
			u32 size = 0;
			u32 part_upgrade = 1;
			char strbuf[512];
			const char *filename = NULL;
			const char *filepath = NULL;
			const char *partname = NULL;
			uint32_t filesize = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &offset);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);

			if (!part_upgrade)
				continue;

			filepath = get_file_path(&pa, filename);
			filesize = get_filesize(filepath);
			if (filesize > size) {
				printf("Error: The size of %s is %d(0x%x), which is bigger than partition size %d(0x%x)!\n", filename, filesize, filesize, size, size);
				printf("Error: Please increase the partitions size or make the file %s smaller!\n", filename);
				exit(1);
			}

			if (partname && !strncmp(partname, pa.part_label_persistentmem, max(strlen(partname), strlen(pa.part_label_persistentmem)))) {
				h.ignore_version_update = 1;
				filename_persistentmem = filename;
				partname_persistentmem = partname;
				memset(&e_persistentmem, 0, sizeof(e_persistentmem));
				e_persistentmem.upgrade.dev_index = -1;
				e_persistentmem.upgrade.dev_type = IH_DEVT_SPINOR1;
				e_persistentmem.upgrade.upgrade_enable = 1;
				e_persistentmem.upgrade.erase_length = size;
				e_persistentmem.upgrade.entry_type = IH_ENTTRY_NORMAL;
				e_persistentmem.upgrade.offset_in_dev = offset;
				/* to be appended at the last entry */
				continue;
			}

			memset(&e, 0, sizeof(e));
			e.upgrade.length = filesize;
			e.upgrade.index = entry_number;
			e.upgrade.dev_index = -1;
			e.upgrade.dev_type = IH_DEVT_SPINOR1;
			e.upgrade.upgrade_enable = 1;
			e.upgrade.offset_in_payload = offset_in_payload;
			e.upgrade.erase_length = size;
			e.upgrade.entry_type = IH_ENTTRY_NORMAL;
			e.upgrade.offset_in_dev = offset;
			if (filesize > 0) {
				payload = realloc(payload, offset_in_payload + filesize);
				FILE *fp = fopen(filepath, "rb");
				fseek(fp, 0, SEEK_SET);
				int ret = fread(payload + offset_in_payload, 1, filesize, fp);
				if (ret != filesize)
					die("Error read %s!\n", filepath);
				fclose(fp);
			}

			snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
			ph.entry[entry_number] = e;
			print_entry_info(&e, &entry_info);
			offset_in_payload += filesize;
			entry_number++;
		}
	} while (0);

	do {
		int np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
		if (np < 0)
			break;

		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash/partitions");
		u32 npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (unsigned i = 1; i <= npart; i++) {
			u32 offset = 0;
			u32 size = 0;
			u32 part_upgrade = 1;
			char strbuf[512];
			const char *filename = NULL;
			const char *filepath = NULL;
			const char *partname = NULL;
			uint32_t filesize = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &offset);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);

			if (!part_upgrade)
				continue;

			filepath = get_file_path(&pa, filename);
			filesize = get_filesize(filepath);
			if (filesize > size) {
				printf("Error: The size of %s is %d(0x%x), which is bigger than partition size %d(0x%x)!\n", filename, filesize, filesize, size, size);
				printf("Error: Please increase the partitions size or make the file %s smaller!\n", filename);
				exit(1);
			}

			if (partname && !strncmp(partname, pa.part_label_persistentmem, max(strlen(partname), strlen(pa.part_label_persistentmem)))) {
				h.ignore_version_update = 1;
				filename_persistentmem = filename;
				partname_persistentmem = partname;
				memset(&e_persistentmem, 0, sizeof(e_persistentmem));
				e_persistentmem.upgrade.dev_index = -1;
				e_persistentmem.upgrade.dev_type = IH_DEVT_SPINAND;
				e_persistentmem.upgrade.upgrade_enable = 1;
				e_persistentmem.upgrade.erase_length = size;
				e_persistentmem.upgrade.entry_type = IH_ENTTRY_NORMAL;
				e_persistentmem.upgrade.offset_in_dev = offset;
				/* to be appended at the last entry */
				continue;
			}

			memset(&e, 0, sizeof(e));
			e.upgrade.length = filesize;
			e.upgrade.index = entry_number;
			e.upgrade.dev_index = -1;
			e.upgrade.dev_type = IH_DEVT_SPINAND;
			e.upgrade.upgrade_enable = 1;
			e.upgrade.offset_in_payload = offset_in_payload;
			e.upgrade.erase_length = size;
			e.upgrade.entry_type = IH_ENTTRY_NORMAL;
			e.upgrade.offset_in_dev = offset;
			if (filesize > 0) {
				payload = realloc(payload, offset_in_payload + filesize);
				FILE *fp = fopen(filepath, "rb");
				fseek(fp, 0, SEEK_SET);
				int ret = fread(payload + offset_in_payload, 1, filesize, fp);
				if (ret != filesize)
					die("Error read %s!\n", filepath);
				fclose(fp);
			}

			snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
			ph.entry[entry_number] = e;
			print_entry_info(&e, &entry_info);
			offset_in_payload += filesize;
			entry_number++;
		}
	} while (0);

	do {
		int np = fdt_get_node_offset_by_path("/hcrtos/nand");
		if (np < 0)
			break;

		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/nand/partitions");
		u32 npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (unsigned i = 1; i <= npart; i++) {
			u32 offset = 0;
			u32 size = 0;
			u32 part_upgrade = 1;
			char strbuf[512];
			const char *filename = NULL;
			const char *filepath = NULL;
			const char *partname = NULL;
			uint32_t filesize = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &offset);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);

			if (!part_upgrade)
				continue;

			filepath = get_file_path(&pa, filename);
			filesize = get_filesize(filepath);
			if (filesize > size) {
				printf("Error: The size of %s is %d(0x%x), which is bigger than partition size %d(0x%x)!\n", filename, filesize, filesize, size, size);
				printf("Error: Please increase the partitions size or make the file %s smaller!\n", filename);
				exit(1);
			}

			if (partname && !strncmp(partname, pa.part_label_persistentmem, max(strlen(partname), strlen(pa.part_label_persistentmem)))) {
				h.ignore_version_update = 1;
				filename_persistentmem = filename;
				partname_persistentmem = partname;
				memset(&e_persistentmem, 0, sizeof(e_persistentmem));
				e_persistentmem.upgrade.dev_index = -1;
				e_persistentmem.upgrade.dev_type = IH_DEVT_NAND;
				e_persistentmem.upgrade.upgrade_enable = 1;
				e_persistentmem.upgrade.erase_length = size;
				e_persistentmem.upgrade.entry_type = IH_ENTTRY_NORMAL;
				e_persistentmem.upgrade.offset_in_dev = offset;
				/* to be appended at the last entry */
				continue;
			}

			memset(&e, 0, sizeof(e));
			e.upgrade.length = filesize;
			e.upgrade.index = entry_number;
			e.upgrade.dev_index = -1;
			e.upgrade.dev_type = IH_DEVT_NAND;
			e.upgrade.upgrade_enable = 1;
			e.upgrade.offset_in_payload = offset_in_payload;
			e.upgrade.erase_length = size;
			e.upgrade.entry_type = IH_ENTTRY_NORMAL;
			e.upgrade.offset_in_dev = offset;
			if (filesize > 0) {
				payload = realloc(payload, offset_in_payload + filesize);
				FILE *fp = fopen(filepath, "rb");
				fseek(fp, 0, SEEK_SET);
				int ret = fread(payload + offset_in_payload, 1, filesize, fp);
				if (ret != filesize)
					die("Error read %s!\n", filepath);
				fclose(fp);
			}

			snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
			ph.entry[entry_number] = e;
			print_entry_info(&e, &entry_info);
			offset_in_payload += filesize;
			entry_number++;
		}
	} while (0);

	do {
		int np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
		if (np < 0)
			break;

		char hgptbuf[4096] = { 0 };
  		struct hgpt_ptable_s *ptbl = (struct hgpt_ptable_s *)hgptbuf;
		ptbl->magic = HGPT_MAGIC;

		np = fdt_get_node_offset_by_path("/hcrtos/external_partitions/sd_mmc");
		unsigned long long totalsize = 0;
		const char *totalsizeStr = NULL;
		if (np >= 0) {
			fdt_get_property_string_index(np, "size", 0, &totalsizeStr);
			if (totalsizeStr) {
				char *s = (char *)totalsizeStr;
				totalsize = memparse(s, &s);
			}
		}

		np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");

		char tmp[256];
		int mkgpt_argc = 0;
		char *mkgpt_argv[100] = { 0 };

		mkgpt_argv[mkgpt_argc++] = (char *)strdup("mkgpt");

		unsigned long long offset = 0;

		/* GPT partition */
		offset += 0x100000;
		mkgpt_argv[mkgpt_argc++] = (char *)strdup("-a");
		mkgpt_argv[mkgpt_argc++] = (char *)strdup("0x100000");

		u32 npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);
		for (unsigned i = 1; i <= npart; i++) {
			u32 size = 0;
			u32 part_upgrade = 1;
			char strbuf[512];
			bool hide = 0;
			const char *offsetstr = NULL;
			const char *partname = NULL;
			const char *filename = NULL;
			const char *filepath = NULL;
			const char *strsize = NULL;
			u32 filesize = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-size", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-strsize", i);
			fdt_get_property_string_index(np, strbuf, 0, &strsize);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			snprintf(strbuf, sizeof(strbuf), "part%d-hide", i);
			hide = fdt_property_read_bool(np, strbuf);
			snprintf(strbuf, sizeof(strbuf), "part%d-offset", i);
			fdt_get_property_string_index(np, strbuf, 0, &offsetstr);

			if (offsetstr) {
				char *s = (char *)offsetstr;
				offset = memparse(s, &s);
			}

			unsigned long long llsize = size;
			if (llsize == 0) {
				if (strsize) {
					char *s = (char *)strsize;
					llsize = memparse(s, &s);
				}
			}
			if (llsize == 0) {
				if (totalsize == 0) {
					die("No total size for the sdmmc, can not accept partition size 0 for %s\r\n", partname);
					exit(1);
				}
				llsize = totalsize - offset - 0x100000; /* remain 1MB at the last for the backup GPT */
			}
			if (llsize > INT_MAX)
				size = INT_MAX;

			if (hide) {
				struct hgpt_header_s hgpt = { 0 };
				hgpt.offset = (uint32_t)offset;
				hgpt.size = (uint32_t)llsize;
				strncat(hgpt.name, partname, sizeof(hgpt.name) - 1);
				ptbl->gpt_header[ptbl->num_partition_entries++] = hgpt;
			} else {
				sprintf(tmp, "%llu@%llu(%s)", llsize, offset, partname);
				mkgpt_argv[mkgpt_argc++] = (char *)strdup("-p");
				mkgpt_argv[mkgpt_argc++] = (char *)strdup(tmp);
			}

			if (!part_upgrade) {
				offset += llsize;
				continue;
			}

			filepath = get_file_path(&pa, filename);
			filesize = get_filesize(filepath);
			if (filesize > llsize) {
				printf("Error: The size of %s is %d(0x%x), which is bigger than partition size %lld(0x%llx)!\n", filename, filesize, filesize, llsize, llsize);
				printf("Error: Please increase the partitions size or make the file %s smaller!\n", filename);
				exit(1);
			}

			if (partname && !strncmp(partname, pa.part_label_persistentmem, max(strlen(partname), strlen(pa.part_label_persistentmem)))) {
				h.ignore_version_update = 1;
				filename_persistentmem = filename;
				partname_persistentmem = partname;
				memset(&e_persistentmem, 0, sizeof(e_persistentmem));
				e_persistentmem.upgrade.dev_index = -1;
				e_persistentmem.upgrade.dev_type = IH_DEVT_EMMC;
				e_persistentmem.upgrade.upgrade_enable = 1;
				e_persistentmem.upgrade.erase_length = size;
				e_persistentmem.upgrade.entry_type = IH_ENTTRY_NORMAL;
				e_persistentmem.upgrade.offset_in_dev = offset;
				/* to be appended at the last entry */
				offset += llsize;
				continue;
			}

			memset(&e, 0, sizeof(e));
			e.upgrade.length = filesize;
			e.upgrade.index = entry_number;
			e.upgrade.dev_index = -1;
			e.upgrade.dev_type = IH_DEVT_EMMC;
			e.upgrade.upgrade_enable = (filesize == 0) ? 0 : 1;
			e.upgrade.offset_in_payload = offset_in_payload;
			e.upgrade.erase_length = size;
			e.upgrade.entry_type = IH_ENTTRY_NORMAL;
			e.upgrade.offset_in_dev = offset;
			if (filesize > 0) {
				payload = realloc(payload, offset_in_payload + filesize);
				FILE *fp = fopen(filepath, "rb");
				fseek(fp, 0, SEEK_SET);
				int ret = fread(payload + offset_in_payload, 1, filesize, fp);
				if (ret != filesize)
					die("Error read %s!\n", filepath);
				fclose(fp);
			}

			snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
			ph.entry[entry_number] = e;
			print_entry_info(&e, &entry_info);
			offset_in_payload += filesize;
			offset += llsize;
			entry_number++;
		}

		if (totalsize == 0)
			totalsize = offset;

		/* GPT */
		const char *partname = "GPT";
		const char *filename = "SDMMC.gpt";
		const char *filepath = get_file_path(&pa, filename);
		sprintf(tmp, "%s", filepath);
		mkgpt_argv[mkgpt_argc++] = (char *)strdup("-d");
		mkgpt_argv[mkgpt_argc++] = (char *)strdup(tmp);
		if (totalsizeStr)
			sprintf(tmp, "%s", totalsizeStr);
		else
			sprintf(tmp, "%llu", totalsize);
		mkgpt_argv[mkgpt_argc++] = (char *)strdup("-s");
		mkgpt_argv[mkgpt_argc++] = (char *)strdup(tmp);
		print_args(mkgpt_argc, mkgpt_argv);
		mkgpt(mkgpt_argc, mkgpt_argv);

		uint32_t filesize = get_filesize(filepath);
		memset(&e, 0, sizeof(e));
		e.upgrade.length = filesize;
		e.upgrade.index = entry_number;
		e.upgrade.dev_index = -1;
		e.upgrade.dev_type = IH_DEVT_EMMC;
		e.upgrade.upgrade_enable = 1;
		e.upgrade.offset_in_payload = offset_in_payload;
		e.upgrade.erase_length = 0x100000 - 4096;
		e.upgrade.entry_type = IH_ENTTRY_NORMAL;
		e.upgrade.offset_in_dev = 0;
		if (filesize > 0) {
			payload = realloc(payload, offset_in_payload + filesize);
			FILE *fp = fopen(filepath, "rb");
			fseek(fp, 0, SEEK_SET);
			int ret = fread(payload + offset_in_payload, 1, filesize, fp);
			if (ret != filesize)
				die("Error read %s!\n", filepath);
			fclose(fp);
		}
		snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname);
		ph.entry[entry_number] = e;
		print_entry_info(&e, &entry_info);
		offset_in_payload += filesize;
		entry_number++;

		/* HGPT for the hidden partitions */
		filesize = sizeof(*ptbl) + ptbl->num_partition_entries * sizeof(ptbl->gpt_header[0]);
		filesize = ((filesize + 511) / 512 ) * 512;
		memset(&e, 0, sizeof(e));
		e.upgrade.length = filesize;
		e.upgrade.index = entry_number;
		e.upgrade.dev_index = -1;
		e.upgrade.dev_type = IH_DEVT_EMMC;
		e.upgrade.upgrade_enable = 1;
		e.upgrade.offset_in_payload = offset_in_payload;
		e.upgrade.erase_length = 4096;
		e.upgrade.entry_type = IH_ENTTRY_NORMAL;
		e.upgrade.offset_in_dev = 0x100000 - 4096;
		if (filesize > 0) {
			payload = realloc(payload, offset_in_payload + filesize);
			memcpy(payload + offset_in_payload, ptbl, filesize);
		}

		offset_in_payload += filesize;
		snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", "HGPT");
		ph.entry[entry_number] = e;
		print_entry_info(&e, &entry_info);
		entry_number++;

		for (int i = 0; i < mkgpt_argc; i++)
			free(mkgpt_argv[i]);
	} while (0);

	/* process the persistentmem partition */
	if (partname_persistentmem) {
		const char *filepath = NULL;
		uint32_t filesize = 0;
		filepath = get_file_path(&pa, filename_persistentmem);
		filesize = get_filesize(filepath);
		e_persistentmem.upgrade.length = filesize;
		e_persistentmem.upgrade.index = entry_number;
		e_persistentmem.upgrade.offset_in_payload = offset_in_payload;
		if (filesize > 0) {
			payload = realloc(payload, offset_in_payload + filesize);
			FILE *fp = fopen(filepath, "rb");
			fseek(fp, 0, SEEK_SET);
			int ret = fread(payload + offset_in_payload, 1, filesize, fp);
			if (ret != filesize)
				die("Error read %s!\n", filepath);
			fclose(fp);
		}

		offset_in_payload += filesize;
		snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "%s", partname_persistentmem);
		ph.entry[entry_number] = e_persistentmem;
		print_entry_info(&e_persistentmem, &entry_info);
		entry_number++;
	}

	/* process the partinfo, only for HCProgrammer to display the part names */
	memset(&e, 0, sizeof(e));
	e.upgrade.length = sizeof(entry_info);
	e.upgrade.index = entry_number;
	e.upgrade.dev_index = -1;
	e.upgrade.dev_type = IH_DEVT_NONE;
	e.upgrade.upgrade_enable = 0;
	e.upgrade.offset_in_payload = offset_in_payload;
	e.upgrade.erase_length = 0;
	e.upgrade.entry_type = IH_ENTTRY_PARTINFO;
	e.upgrade.offset_in_dev = -1;
	snprintf(entry_info.names[entry_number], sizeof(entry_info.names[entry_number]), "partinfo");
	payload = realloc(payload, offset_in_payload + e.upgrade.length);
	memcpy(payload + offset_in_payload, &entry_info, sizeof(entry_info));
	offset_in_payload += e.upgrade.length;
	ph.entry[entry_number] = e;
	print_entry_info(&e, &entry_info);
	entry_number++;

	ph.entry_number = entry_number;

	h.compress_type = IH_COMP_NONE;
	if (pa.for_upgrade) {
		h.ignore_version_check = 0;
	} else {
		h.ignore_version_check = !pa.version_check;
	}

	h.version = pa.version;
	h.erase_nor_chip = pa.erase_nor_chip;
	h.erase_nor1_chip = pa.erase_nor1_chip;
	h.erase_nand_chip = pa.erase_nand_chip;

	h.uncompressed_length = sizeof(ph) + offset_in_payload;
	if (pa.product[0] != '\0')
		snprintf((char *)&h.board[0], sizeof(h.board), "%s", pa.product);
	h.payload_size = sizeof(ph) + offset_in_payload;

	uint32_t crc = crc32(0, (const uint8_t *)&ph, sizeof(ph));
	crc = crc32(crc, (const uint8_t *)payload, offset_in_payload);
	ph.crc = crc;

	crc = crc32(0, (const uint8_t *)&h, sizeof(h));
	crc = crc32(crc, (const uint8_t *)&ph, sizeof(ph));
	crc = crc32(crc, (const uint8_t *)payload, offset_in_payload);
	h.crc = crc;

	size_t fota_size = sizeof(h) + sizeof(ph) + offset_in_payload;
	void *fota = malloc(fota_size);
	memcpy(fota + 0, &h, sizeof(h));
	memcpy(fota + sizeof(h), &ph, sizeof(ph));
	memcpy(fota + sizeof(h) + sizeof(ph), payload, offset_in_payload);

	FILE *fp = fopen(output_path, "wb");
	if (!fp) {
		printf("Error create file %s!\n", output_path);
		return -1;
	}
	fwrite(fota, fota_size, 1, fp);
	fclose(fp);

	if (output_path2) {
		fp = fopen(output_path2, "wb");
		if (!fp) {
			printf("Error create file %s!\n", output_path2);
		}

		fwrite(fota, fota_size, 1, fp);
		fclose(fp);
	}

	printf("header->ignore_version_check  : %s\r\n", h.ignore_version_check ? "TRUE" : "FALSE");
	printf("header->ignore_version_update : %s\r\n", h.ignore_version_update ? "TRUE" : "FALSE");
	printf("header->spinor_en_cs0         : %s\r\n", h.spinor_en_cs0 ? "TRUE" : "FALSE");
	printf("header->spinor_en_cs1         : %s\r\n", h.spinor_en_cs1 ? "TRUE" : "FALSE");
	printf("header->spinand_en_cs0        : %s\r\n", h.spinand_en_cs0 ? "TRUE" : "FALSE");
	printf("header->spinand_en_cs1        : %s\r\n", h.spinand_en_cs1 ? "TRUE" : "FALSE");
	printf("header->nand_en               : %s\r\n", h.nand_en ? "TRUE" : "FALSE");
	printf("header->emmc_v20_left_en      : %s\r\n", h.emmc_v20_left_en ? "TRUE" : "FALSE");
	printf("header->emmc_v20_top_en       : %s\r\n", h.emmc_v20_top_en ? "TRUE" : "FALSE");
	printf("header->emmc_v30_en           : %s\r\n", h.emmc_v30_en ? "TRUE" : "FALSE");

	printf("Generate %s done!\r\n", output_path);
	if (output_path2)
		printf("Generate %s done!\r\n", output_path2);

	return 0;
}
