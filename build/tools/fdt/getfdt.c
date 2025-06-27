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

int main(int argc, char *argv[])
{
	char *dtb_path = argv[1];
	char *node_path = argv[2];
	char *type = argv[3];
	char *prop = argv[4];
	char *dtb;
	size_t len;

	dtb = utilfdt_read(dtb_path, &len);
	if (!dtb)
		die("could not read: %s\n", dtb_path);

	fdt_setup(dtb);

	if (!strcmp(type, "u32") ) {
		int np = fdt_get_node_offset_by_path(node_path);
		if (np >= 0) {
			u32 val = 0;
			if (fdt_get_property_u_32_index(np, prop, 0, &val) == 0) {
				printf("%u", val);
				return 0;
			}
		}
	} else if (!strcmp(type, "string") ) {
		int np = fdt_get_node_offset_by_path(node_path);
		if (np >= 0) {
			const char *str = NULL;
			if (fdt_get_property_string_index(np, prop, 0, &str) == 0) {
				printf("%s", str);
				return 0;
			}
		}
	}

	exit(1);
}
