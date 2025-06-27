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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libfdt.h>
#include <libfdt_env.h>
#include <fdt.h>
#include "fdt_api.h"

#include "util.h"

#define max(a, b)                                                              \
	({                                                                     \
		typeof(a) _a = a;                                              \
		typeof(b) _b = b;                                              \
		_a > _b ? _a : _b;                                             \
	})

#define FDT_MAGIC_SIZE	4
#define MAX_VERSION 17

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)	((void *)(ALIGN((unsigned long)(p), (a))))
#define GET_CELL(p)	(p += 4, *((const fdt32_t *)(p-4)))

static void print_usage(const char *prog)
{
	printf("Usage: %s [-iow]\n", prog);
	puts("  -d --dtb      input dtb file\n"
	     "  -o --outdir   output directory\n"
	     "  -w --wkdir    working directory\n"
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

static int get_filesize(const char *file)
{
	struct stat sb;

	if (stat(file, &sb) == -1)
		return (int)-1;
	return (int)sb.st_size;
}

int main(int argc, char *argv[])
{
	const char *dtb_path = NULL;
	const char *wkdir = NULL;
	const char *outdir = NULL;
	char *dtb;
	size_t len;
	FILE *fp_spinor;
	FILE *fp_spinand;
	FILE *fp_spinand_partinfo;
	FILE *fp_nand;
	FILE *fp_nand_partinfo;
	FILE *fp;
	const char *status = NULL;
	int i, np;
	uint32_t npart = 0;
	char strbuf[512];
	char cmdbuf[512];
	uint32_t start = 0, size = 0, total_size, file_size;
	const char *partname;
	const char *filename = "null";
	char *buf;
	size_t rc;

	while (1) {
		static const struct option lopts[] = {
			{ "dtb",     1, 0, 'd' },
			{ "outdir",  1, 0, 'o' },
			{ "wkdir",   1, 0, 'w' },
			{ "help",    0, 0, 'h' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "d:o:w:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			dtb_path = optarg;
			break;
		case 'o':
			outdir = optarg;
			break;
		case 'w':
			wkdir = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!dtb_path || !wkdir) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	dtb = utilfdt_read(dtb_path, &len);
	if (!dtb)
		die("could not read: %s\n", dtb_path);

	if (!valid_header(dtb, len))
		die("%s: header is not valid\n", dtb_path);

	fdt_setup(dtb);

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

		if (npart == 0)
			break;

		total_size = 0;
		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename)) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (filename) {
				memset(strbuf, 0, sizeof(strbuf));
				snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
				file_size = get_filesize(strbuf);
				if (file_size == -1) {
					printf("File %s not found!\n", strbuf);
					return -1;
				}

				if (file_size > size) {
					printf("File %s size %d(0x%x) is bigger than partition size %d(0x%x)!\n",
					       filename, file_size, file_size, size, size);
					return -1;
				}
			}

			total_size = max(total_size, start + size);
		}
		if (total_size == 0)
			break;

		buf = malloc(total_size);
		memset(buf, 0xff, total_size);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename))
				continue;
			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				continue;
			}

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
			file_size = get_filesize(strbuf);
			fp = fopen(strbuf, "rb");
			rc = fread(buf + start, 1, file_size, fp);
			if (rc != file_size) {
				printf("Read %s failed!\n", strbuf);
				fclose(fp);
				return -1;
			}
			fclose(fp);
		}

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/spinorflash.bin", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/spinorflash.bin", wkdir);
		fp_spinor = fopen(strbuf, "wb");
		if (!fp_spinor) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}

		rc = fwrite(buf, 1, total_size, fp_spinor);
		if (rc != total_size) {
			printf("Write %s failed!\n", strbuf);
			return -1;
		}
		fflush(fp_spinor);
		fclose(fp_spinor);
		printf("Generate spinorflash.bin done, total size %d(0x%x)!\n", total_size, total_size);
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

		if (npart == 0)
			break;

		total_size = 0;
		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename)) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (filename) {
				memset(strbuf, 0, sizeof(strbuf));
				snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
				file_size = get_filesize(strbuf);
				if (file_size == -1) {
					printf("File %s not found!\n", strbuf);
					return -1;
				}

				if (file_size > size) {
					printf("File %s size %d(0x%x) is bigger than partition size %d(0x%x)!\n",
					       filename, file_size, file_size, size, size);
					return -1;
				}
			}

			total_size = max(total_size, start + size);
		}
		if (total_size == 0)
			break;

		buf = malloc(total_size);
		memset(buf, 0xff, total_size);

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename))
				continue;
			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				continue;
			}

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
			file_size = get_filesize(strbuf);
			fp = fopen(strbuf, "rb");
			rc = fread(buf + start, 1, file_size, fp);
			if (rc != file_size) {
				printf("Read %s failed!\n", strbuf);
				fclose(fp);
				return -1;
			}
			fclose(fp);
		}

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/spinorflash1.bin", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/spinorflash1.bin", wkdir);
		fp_spinor = fopen(strbuf, "wb");
		if (!fp_spinor) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}

		rc = fwrite(buf, 1, total_size, fp_spinor);
		if (rc != total_size) {
			printf("Write %s failed!\n", strbuf);
			return -1;
		}
		fflush(fp_spinor);
		fclose(fp_spinor);
		printf("Generate spinorflash1.bin done, total size %d(0x%x)!\n", total_size, total_size);
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

		if (npart == 0)
			break;

		total_size = 0;
		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename)) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (filename) {
				memset(strbuf, 0, sizeof(strbuf));
				snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
				file_size = get_filesize(strbuf);
				if (file_size == -1) {
					printf("File %s not found!\n", strbuf);
					return -1;
				}

				if (file_size > size) {
					printf("File %s size %d(0x%x) is bigger than partition size %d(0x%x)!\n",
					       filename, file_size, file_size, size, size);
					return -1;
				}
			}

			total_size = max(total_size, start + size);
		}
		if (total_size == 0)
			break;

		buf = malloc(total_size);
		memset(buf, 0xff, total_size);

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/spinandflash.partinfo.txt", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/spinandflash.partinfo.txt", wkdir);
		fp_spinand_partinfo = fopen(strbuf, "w");
		if (!fp_spinand_partinfo) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}
		fprintf(fp_spinand_partinfo, "start        size     filename\n");

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename))
				continue;
			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				continue;
			}

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
			file_size = get_filesize(strbuf);
			fp = fopen(strbuf, "rb");
			rc = fread(buf + start, 1, file_size, fp);
			if (rc != file_size) {
				printf("Read %s failed!\n", strbuf);
				fclose(fp);
				return -1;
			}
			fclose(fp);

			fprintf(fp_spinand_partinfo, "0x%x    0x%x    %s\n", start, size, filename);

			if (outdir) {
				memset(cmdbuf, 0, sizeof(cmdbuf));
				snprintf(cmdbuf, sizeof(cmdbuf), "cp -vf %s/%s %s/%s", wkdir, filename, outdir, filename);
				rc = system(cmdbuf);
			}
		}

		fclose(fp_spinand_partinfo);

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/spinandflash.bin", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/spinandflash.bin", wkdir);
		fp_spinand = fopen(strbuf, "wb");
		if (!fp_spinand) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}

		rc = fwrite(buf, 1, total_size, fp_spinand);
		if (rc != total_size) {
			printf("Write %s failed!\n", strbuf);
			return -1;
		}
		fflush(fp_spinand);
		fclose(fp_spinand);
		printf("Generate spinandflash.bin done, total size %d(0x%x)!\n", total_size, total_size);
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

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash/partitions");
		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		if (npart == 0)
			break;

		total_size = 0;
		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename)) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				total_size = max(total_size, start + size);
				continue;
			}

			if (filename) {
				memset(strbuf, 0, sizeof(strbuf));
				snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
				file_size = get_filesize(strbuf);
				if (file_size == -1) {
					printf("File %s not found!\n", strbuf);
					return -1;
				}

				if (file_size > size) {
					printf("File %s size %d(0x%x) is bigger than partition size %d(0x%x)!\n",
					       filename, file_size, file_size, size, size);
					return -1;
				}
			}

			total_size = max(total_size, start + size);
		}
		if (total_size == 0)
			break;

		buf = malloc(total_size);
		memset(buf, 0xff, total_size);

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/nandflash.partinfo.txt", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/nandflash.partinfo.txt", wkdir);
		fp_nand_partinfo = fopen(strbuf, "w");
		if (!fp_nand_partinfo) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}
		fprintf(fp_nand_partinfo, "start        size     filename\n");

		for (i = 1; i <= npart; i++) {
			start = 0;
			size = 0;
			filename = "null";

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, &start);
			fdt_get_property_u_32_index(np, strbuf, 1, &size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &filename))
				continue;
			if (!strncasecmp(filename, "null", max(strlen(filename), 4))) {
				continue;
			}

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "%s/%s", wkdir, filename);
			file_size = get_filesize(strbuf);
			fp = fopen(strbuf, "rb");
			rc = fread(buf + start, 1, file_size, fp);
			if (rc != file_size) {
				printf("Read %s failed!\n", strbuf);
				fclose(fp);
				return -1;
			}
			fclose(fp);

			fprintf(fp_nand_partinfo, "0x%x    0x%x    %s\n", start, size, filename);

			if (outdir) {
				memset(cmdbuf, 0, sizeof(cmdbuf));
				snprintf(cmdbuf, sizeof(cmdbuf), "cp -vf %s/%s %s/%s", wkdir, filename, outdir, filename);
				rc = system(cmdbuf);
			}
		}

		fclose(fp_nand_partinfo);

		memset(strbuf, 0, sizeof(strbuf));
		if (outdir)
			snprintf(strbuf, sizeof(strbuf), "%s/nandflash.bin", outdir);
		else
			snprintf(strbuf, sizeof(strbuf), "%s/nandflash.bin", wkdir);
		fp_nand = fopen(strbuf, "wb");
		if (!fp_nand) {
			printf("Create %s failed!\n", strbuf);
			return -1;
		}

		rc = fwrite(buf, 1, total_size, fp_nand);
		if (rc != total_size) {
			printf("Write %s failed!\n", strbuf);
			return -1;
		}
		fflush(fp_nand);
		fclose(fp_nand);
		printf("Generate nandflash.bin done, total size %d(0x%x)!\n", total_size, total_size);
	} while (0);

	free(dtb);

	return 0;
}
