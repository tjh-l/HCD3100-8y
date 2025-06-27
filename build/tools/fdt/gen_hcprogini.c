// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * fdtdump.c - Contributed by Pantelis Antoniou <pantelis.antoniou AT gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
#include "fdt_api.h"

#include "util.h"

#define FDT_MAGIC_SIZE	4
#define MAX_VERSION 17

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)	((void *)(ALIGN((unsigned long)(p), (a))))
#define GET_CELL(p)	(p += 4, *((const fdt32_t *)(p-4)))

static void print_usage(const char *prog)
{
	printf("Usage: %s [-docivp]\n", prog);
	puts("  -d --dtb      input dtb file\n"
	     "  -o --output   output file\n"
	     "  -c --chip     chip type\n"
	     "  -i --draminit dram init file\n"
	     "  -v --version  version\n"
	     "  -p --product  product\n"
	     "  -u --updater  updater file\n"
	     "  -h --help     print this help\n");
}

static bool valid_header(char *p, off_t len)
{
	if (len < sizeof(struct fdt_header) ||
	    fdt_magic(p) != FDT_MAGIC ||
	    fdt_version(p) > MAX_VERSION ||
	    fdt_last_comp_version(p) > MAX_VERSION ||
	    fdt_totalsize(p) >= len ||
	    fdt_off_dt_struct(p) >= len ||
	    fdt_off_dt_strings(p) >= len)
		return 0;
	else
		return 1;
}

int main(int argc, char *argv[])
{
	const char *dtb_path = NULL;
	const char *output = NULL;
	const char *chip = NULL;
	const char *draminit = NULL;
	const char *version = NULL;
	const char *product = NULL;
	const char *updater = NULL;
	char *dtb;
	size_t len;
	FILE *fp;
	const char *status = NULL;
	int i, np, idx = 0;
	uint32_t npart = 0;
	char strbuf[512];
	uint32_t start = 0, size = 0, part_upgrade = 1;
	const char *partname;
	const char *filename = "null";

	while (1) {
		static const struct option lopts[] = {
			{ "dtb",     1, 0, 'd' },
			{ "output",  1, 0, 'o' },
			{ "chip",    1, 0, 'c' },
			{ "draminit",1, 0, 'i' },
			{ "version", 1, 0, 'v' },
			{ "product", 1, 0, 'p' },
			{ "updater", 1, 0, 'u' },
			{ "help",    0, 0, 'h' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "d:o:c:i:v:p:u:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			dtb_path = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 'c':
			chip = optarg;
			break;
		case 'i':
			draminit = optarg;
			break;
		case 'u':
			updater = optarg;
			break;
		case 'v':
			version = optarg;
			break;
		case 'p':
			product = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!dtb_path || !output || !chip || !draminit || !version || !product) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	dtb = utilfdt_read(dtb_path, &len);
	if (!dtb)
		die("could not read: %s\n", dtb_path);

	if (!valid_header(dtb, len))
		die("%s: header is not valid\n", dtb_path);

	fp = fopen(output, "w");
	if (!fp) {
		perror("fopen");
		free(dtb);
		return EXIT_FAILURE;
	}

	fdt_setup(dtb);

	fprintf(fp, "\n"
		"[SYSINI]\n"
		"HICHIP = %s\n"
		"\n"
		"[STARTUP-FILE]\n"
		"DRAM = %s\n"
		"UPDATER = %s\n"
		"UPDATER_LOAD_ADDR = 0xA0000200\n"
		"UPDATER_RUN_ADDR  = 0xA0000200\n"
		"\n"
		"[NORFLASH-DEFAULT-CONFIG]\n"
		"NORF_TOTAL_SIZE = 0x2000000\n"
		"NORF_ERASE_SIZE = 0x10000\n"
		"NORF_SECTOR_SIZE= 0x10000\n"
		"NORF_PAGE_SIZE  = 0x100\n"
		"\n"
		"[SYSINFO]\n"
		"VERSION = %s\n"
		"PRODUCT = %s\n"
		, chip, draminit, updater, version, product);

	np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
	if (np < 0) {
		fprintf(fp, "SPINOR_EN_CS0 = FALSE\n");
	} else {
		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			fprintf(fp, "SPINOR_EN_CS0 = FALSE\n");
		} else {
			fprintf(fp, "SPINOR_EN_CS0 = TRUE\n");
		}
	}

	np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1");
	if (np < 0) {
		fprintf(fp, "SPINOR_EN_CS1 = FALSE\n");
	} else {
		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			fprintf(fp, "SPINOR_EN_CS1 = FALSE\n");
		} else {
			fprintf(fp, "SPINOR_EN_CS1 = TRUE\n");
		}
	}

	np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
	if (np < 0) {
		fprintf(fp, "SPINAND_EN_CS0 = FALSE\n");
		fprintf(fp, "SPINAND_EN_CS1 = FALSE\n");
	} else {
		status = NULL;
		uint32_t reg = 0;
		if (fdt_get_property_u_32_index(np, "reg", 0, &reg) != 0)
			reg = 0;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			fprintf(fp, "SPINAND_EN_CS0 = FALSE\n");
			fprintf(fp, "SPINAND_EN_CS1 = FALSE\n");
		} else {
			for (i = 0; i < 2; i++) {
				if (i == reg)
					fprintf(fp, "SPINAND_EN_CS%d = TRUE\n", i);
				else
					fprintf(fp, "SPINAND_EN_CS%d = FALSE\n", i);
			}
		}
	}

	np = fdt_get_node_offset_by_path("/hcrtos/nand");
	if (np < 0) {
		fprintf(fp, "NAND_EN = FALSE\n");
	} else {
		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			fprintf(fp, "NAND_EN = FALSE\n");
		} else {
			fprintf(fp, "NAND_EN = TRUE\n");
		}
	}

	np = fdt_get_node_offset_by_path("/hcrtos/mmc");
	if (np < 0) {
		fprintf(fp, "EMMC_V20_LEFT_EN = FALSE\n");
		fprintf(fp, "EMMC_V20_TOP_EN = FALSE\n");
		fprintf(fp, "EMMC_V30_EN = FALSE\n");
	} else {
		uint32_t pinpad = 0;
		if (fdt_get_property_u_32_index(np, "pinmux-active", 0, &pinpad) != 0)
			pinpad = 0;
		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			fprintf(fp, "EMMC_V20_LEFT_EN = FALSE\n");
			fprintf(fp, "EMMC_V20_TOP_EN = FALSE\n");
			fprintf(fp, "EMMC_V30_EN = FALSE\n");
		} else {
			if (!strcmp(chip, "H16XX")) {
				fprintf(fp, "EMMC_V20_LEFT_EN = FALSE\n");
				fprintf(fp, "EMMC_V20_TOP_EN = FALSE\n");
				fprintf(fp, "EMMC_V30_EN = TRUE\n");
			} else if (!strcmp(chip, "H15XX")) {
				if (pinpad >= 16 && pinpad <= 21) {
					fprintf(fp, "EMMC_V20_LEFT_EN = TRUE\n");
					fprintf(fp, "EMMC_V20_TOP_EN = FALSE\n");
					fprintf(fp, "EMMC_V30_EN = TRUE\n");
				} else if (pinpad >= 96 && pinpad <= 101) {
					fprintf(fp, "EMMC_V20_LEFT_EN = FALSE\n");
					fprintf(fp, "EMMC_V20_TOP_EN = TRUE\n");
					fprintf(fp, "EMMC_V30_EN = TRUE\n");
				}
			}
		}
	}

	fprintf(fp, "\n");

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
		if (np < 0)
			break;

		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";
			part_upgrade = 1;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			fprintf(fp, "\n"
				"[PARTITION%d]\n"
				"DEVICE = NOR\n"
				"NAME   = %s\n"
				"OFFSET = 0x%08x\n"
				"SIZE   = 0x%08x\n"
				"FILE   = %s\n"
				"UPGRADE = %d\n"
				"\n", idx + i - 1, partname, start, size, filename, !!part_upgrade);
		}
		idx += i - 1;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1");
		if (np < 0)
			break;

		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash1/partitions");
		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";
			part_upgrade = 1;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			fprintf(fp, "\n"
				"[PARTITION%d]\n"
				"DEVICE = NOR1\n"
				"NAME   = %s\n"
				"OFFSET = 0x%08x\n"
				"SIZE   = 0x%08x\n"
				"FILE   = %s\n"
				"UPGRADE = %d\n"
				"\n", idx + i - 1, partname, start, size, filename, !!part_upgrade);
		}
		idx += i - 1;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
		if (np < 0)
			break;

		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash/partitions");
		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";
			part_upgrade = 1;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			fprintf(fp, "\n"
				"[PARTITION%d]\n"
				"DEVICE = SPINAND\n"
				"NAME   = %s\n"
				"OFFSET = 0x%08x\n"
				"SIZE   = 0x%08x\n"
				"FILE   = %s\n"
				"UPGRADE = %d\n"
				"\n", idx + i - 1, partname, start, size, filename, !!part_upgrade);
		}
		idx += i - 1;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/nand");
		if (np < 0)
			break;

		status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/nand/partitions");
		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";
			part_upgrade = 1;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			fprintf(fp, "\n"
				"[PARTITION%d]\n"
				"DEVICE = NAND\n"
				"NAME   = %s\n"
				"OFFSET = 0x%08x\n"
				"SIZE   = 0x%08x\n"
				"FILE   = %s\n"
				"UPGRADE = %d\n"
				"\n", idx + i - 1, partname, start, size, filename, !!part_upgrade);
		}
		idx += i - 1;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
		if (np < 0)
			break;

		start = 0;
		fprintf(fp, "\n"
			"[PARTITION%d]\n"
			"DEVICE = EMMC\n"
			"NAME   = %s\n"
			"OFFSET = 0x%08x\n"
			"SIZE   = 0x%08x\n"
			"FILE   = %s\n"
			"\n", idx, "MBR", start, 0x100000, "EMMC.mbr");
		idx++;
		start += 0x100000;

		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			size = 0;
			filename = "null";
			part_upgrade = 1;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-size", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			snprintf(strbuf, sizeof(strbuf), "part%d-upgrade", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &part_upgrade);
			fprintf(fp, "\n"
				"[PARTITION%d]\n"
				"DEVICE = EMMC\n"
				"NAME   = %s\n"
				"OFFSET = 0x%08x\n"
				"SIZE   = 0x%08x\n"
				"FILE   = %s\n"
				"UPGRADE = %d\n"
				"\n", idx + i - 1, partname, start, size, filename, !!part_upgrade);

			start += size;
		}
		idx += i - 1;
	} while (0);

	fclose(fp);
	free(dtb);

	return 0;
}
