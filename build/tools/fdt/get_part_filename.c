// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * fdtdump.c - Contributed by Pantelis Antoniou <pantelis.antoniou AT gmail.com>
 */

#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libfdt.h>

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <libfdt_env.h>
#include <fdt.h>
#include "fdt_api.h"

#include "util.h"

#define max(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a > _b ? _a : _b; })

static void print_usage(const char *prog)
{
	printf("Usage: %s [il]\n", prog);
	puts("  -i --input    input dtb file\n"
	     "  -l --label    partition lable\n"
	     "  -h --help     print this help\n");
}

int main(int argc, char *argv[])
{
	char *input = NULL;
	const char *label = NULL;
	char *dtb;
	size_t len;
	int i, np;
	const char *status = NULL;
	uint32_t npart = 0;
	char strbuf[128];
	const char *partname;
	const char *filename = "null";

	while (1) {
		static const struct option lopts[] = {
			{ "input",   1, 0, 'i' },
			{ "label",   1, 0, 'l' },
			{ "help",    0, 0, 'h' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "i:l:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'i':
			input = optarg;
			break;
		case 'l':
			label = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!input || !label) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	dtb = utilfdt_read(input, &len);
	if (!dtb) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

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

		for (i = 1; i <= npart; i++) {
			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			fdt_get_property_string_index(np, strbuf, 0, &partname);
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			if (strncmp(partname, label, max(strlen(partname), strlen(label))))
				continue;
			printf("%s", filename);
		}
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
			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			fdt_get_property_string_index(np, strbuf, 0, &partname);
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			if (strncmp(partname, label, max(strlen(partname), strlen(label))))
				continue;
			printf("%s", filename);
		}
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
			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			fdt_get_property_string_index(np, strbuf, 0, &partname);
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			if (strncmp(partname, label, max(strlen(partname), strlen(label))))
				continue;
			printf("%s", filename);
		}
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
		if (np < 0)
			break;

		npart = 0;
		fdt_get_property_u_32_index(np, "part-num", 0, &npart);

		for (i = 1; i <= npart; i++) {
			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			fdt_get_property_string_index(np, strbuf, 0, &partname);
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &filename);
			if (strncmp(partname, label, max(strlen(partname), strlen(label))))
				continue;
			printf("%s", filename);
		}
	} while (0);

	return 0;
}
