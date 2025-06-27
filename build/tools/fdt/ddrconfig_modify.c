#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <ctype.h>
#include <inttypes.h>

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
	printf("Usage: %s [-iosftenpbvP]\n", prog);
	puts("  -i --input    input file\n"
	     "  -o --output   output file\n"
	     "  -s --size     bootloader size\n"
	     "  -f --from     bootloader address in sflash\n"
	     "  -t --to       bootloader address in dram\n"
	     "  -e --entry    bootloader entry address\n"
	     "  -n --nand     spi nand boot\n"
	     "  -E --sdmmc    sdmmc boot\n"
	     "  -p --pagesize spi nand pagesize, default 0x800\n"
	     "  -b --erasesize spi nand erasesize, default 0x20000\n"
	     "  -I --irq      hcprogrammer usb irq detect timeout, default 300 (milliseconds)\n"
	     "  -S --sync     hcprogrammer usb sync detect timeout, default 1000 (milliseconds)\n"
	     "  -A --portA    hcprogrammer support usb port A (port 0), default 1(enable)\n"
	     "  -B --portB    hcprogrammer support usb port B (port 1), default 1(enable)\n"
	     "  -d --dtb      input dtb file\n"
	     "  -v --version  firmware version\n"
	     "  -P --product  product name\n"
	     "  -m --loaddtb  loaddtb enabled for separate dtb of bootloader\n");
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

static unsigned long long memparse(const char *ptr, char **retptr)
{
        char *endptr;   /* local pointer to end of parsed string */

        unsigned long long ret = strtoll(ptr, &endptr, 0);

        switch (*endptr) {
        case 'E':
        case 'e':
                ret <<= 10;
        case 'P':
        case 'p':
                ret <<= 10;
        case 'T':
        case 't':
                ret <<= 10;
        case 'G':
        case 'g':
                ret <<= 10;
        case 'M':
        case 'm':
                ret <<= 10;
        case 'K':
        case 'k':
                ret <<= 10;
                endptr++;
        default:
                break;
        }

        if (retptr)
                *retptr = endptr;

        return ret;
}

static void print_args(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\r\n");
}

int main(int argc, char *argv[])
{
	uint32_t magic;
	uint32_t size = 0, from = 0, to = 0, load_dtb = 0, dtb_from = 0, dtb_to = 0, dtb_size = 0, entry = 0;
	uint32_t is_sdmmc = 0, is_nand = 0, nand_pagesize = 0x800, nand_erasesize = (0x800 * 64);
	char *input = NULL;
	char *output = NULL;
	char *buf = NULL;
	char *ptr = NULL;
	struct stat sb;
	FILE *fpin, *fpout;
	size_t ret = -1;
	uint32_t hcprogrammer_irq_timeout = 300;
	uint32_t hcprogrammer_sync_timeout = 1000;
	uint32_t hcprogrammer_usb0_en = 1;
	uint32_t hcprogrammer_usb1_en = 1;
	uint32_t hcprogrammer_support_winusb = 0;
	uint32_t spi_wire = 1;
	uint32_t spi_sclk = 27000000;
	unsigned short scpu_clk = 0; /* scpu clock selection, default (594 MHz) */
	unsigned short scpu_dpll_clk = 800; /* scpu dpll clock (MHz) */
	unsigned short mcpu_clk = 0; /* mcpu clock selection, default (594 MHz) */
	unsigned short mcpu_dpll_clk = 900; /* mcpu dpll clock (MHz) */
	const char *dtb_path = NULL;
	char *dtb;
	size_t len;
	int np;
	char product_id[16] = { 0 };
	uint32_t firmware_version = 0;

	print_args(argc, argv);
	opterr = 0;
	optind = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "input",  1, 0, 'i' },
			{ "output", 1, 0, 'o' },
			{ "size",   1, 0, 's' },
			{ "from",   1, 0, 'f' },
			{ "to",     1, 0, 't' },
			{ "entry",  1, 0, 'e' },
			{ "nand",  0, 0, 'n' },
			{ "sdmmc",  0, 0, 'E' },
			{ "pagesize",  1, 0, 'p' },
			{ "erasesize",  1, 0, 'b' },
			{ "irq",  1, 0, 'I' },
			{ "sync",  1, 0, 'S' },
			{ "portA",  1, 0, 'A' },
			{ "portB",  1, 0, 'B' },
			{ "dtb",  1, 0, 'd' },
			{ "version",  1, 0, 'v' },
			{ "product",  1, 0, 'P' },
			{ "loaddtb",  1, 0, 'D' },
			{ "winusb",  1, 0, 'w' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "i:o:s:f:t:e:np:b:I:S:A:B:d:v:D:P:m:Ew:", lopts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			size = ((size + 31) / 32) * 32;
			break;
		case 'f':
			from = strtoul(optarg, NULL, 0);
			break;
		case 't':
			to = strtoul(optarg, NULL, 0);
			to |= 0xa0000000;
			break;
		case 'e':
			entry = strtoul(optarg, NULL, 0);
			entry |= 0xa0000000;
			break;
		case 'n':
			is_nand = 1;
			break;
		case 'E':
			is_sdmmc = 1;
			break;
		case 'p':
			nand_pagesize = strtoul(optarg, NULL, 0);
			break;
		case 'b':
			nand_erasesize = strtoul(optarg, NULL, 0);
			break;
		case 'I':
			hcprogrammer_irq_timeout = strtoul(optarg, NULL, 0);
			break;
		case 'S':
			hcprogrammer_sync_timeout = strtoul(optarg, NULL, 0);
			break;
		case 'A':
			hcprogrammer_usb0_en = !!strtoul(optarg, NULL, 0);
			break;
		case 'B':
			hcprogrammer_usb1_en = !!strtoul(optarg, NULL, 0);
			break;
		case 'w':
			hcprogrammer_support_winusb = !!strtoul(optarg, NULL, 0);
			break;
		case 'd':
			dtb_path = optarg;
			break;
		case 'v':
			firmware_version = atoi(optarg);
			break;
		case 'P':
			memset(product_id, 0, sizeof(product_id));
			strncpy(product_id, optarg, sizeof(product_id));
			break;
		case 'D':
			if (strtoul(optarg, NULL, 0))
				load_dtb = 1;
			else
				load_dtb = 0;
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!input) {
		printf("No input file!\n");
		print_usage(argv[0]);
		return -1;
	}

	if (stat(input, &sb) == -1) {
		printf("stat %s fail", input);
		return -1;
	}

	if (!output) {
		fpin = fpout = fopen(input, "wb+");
	} else {
		fpin = fopen(input, "rb");
		fpout = fopen(output, "wb");
	}

	if (dtb_path) {
		dtb = utilfdt_read(dtb_path, &len);
		if (load_dtb) {
			dtb_size = fdt_totalsize(dtb);
			dtb_size += (4 - dtb_size % 4);
		}
		if (!dtb)
			die("could not read: %s\n", dtb_path);

		if (!valid_header(dtb, len))
			die("%s: header is not valid\n", dtb_path);

		fdt_setup(dtb);
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi");
		if (np >= 0) {
			uint32_t sclk = 0;
			fdt_get_property_u_32_index(np, "sclk", 0, &sclk);
			if (sclk != 0)
				spi_sclk = sclk;
		}

		if (load_dtb && is_sdmmc) {
			uint32_t offset = 0x100000;
			np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
			if (np >= 0) {
				u32 npart = 0;
				fdt_get_property_u_32_index(np, "part-num", 0, &npart);
				for (unsigned i = 1; i <= npart; i++) {
					u32 partsize = 0;
					char strbuf[512];
					const char *offsetstr = NULL;
					const char *partname = NULL;
					const char *strsize = NULL;

					snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
					if (fdt_get_property_string_index(np, strbuf, 0, &partname))
						continue;
					memset(strbuf, 0, sizeof(strbuf));
					snprintf(strbuf, sizeof(strbuf), "part%d-size", i);
					fdt_get_property_u_32_index(np, strbuf, 0, &partsize);
					snprintf(strbuf, sizeof(strbuf), "part%d-strsize", i);
					fdt_get_property_string_index(np, strbuf, 0, &strsize);
					snprintf(strbuf, sizeof(strbuf), "part%d-offset", i);
					fdt_get_property_string_index(np, strbuf, 0, &offsetstr);

					if (offsetstr) {
						char *s = (char *)offsetstr;
						offset = memparse(s, &s);
					}

					unsigned long long llsize = partsize;
					if (llsize == 0) {
						if (strsize) {
							char *s = (char *)strsize;
							llsize = memparse(s, &s);
						}
					}
					if (llsize == 0 || llsize > INT_MAX) {
						die("No dtb partition found, but found 0 size for the partition %s\r\n", partname);
						exit(1);
					}
					if (strcmp(partname, "dtb")) {
						offset += llsize;
						continue;
					}

					dtb_from = offset;
					break;
				}
			}
			np = fdt_get_node_offset_by_path("/hcrtos/memory-mapping/bootmem");
			if (np >= 0) {
				fdt_get_property_u_32_index(np, "reg", 0, &dtb_to);
				if (dtb_to != 0) {
					dtb_to |= 0xa0000000;
					dtb_to -= dtb_size;
				}
			}
			if (dtb_from == 0 || dtb_size == 0 || dtb_to == 0)
				die("bootloader needs a separate DTB but the DTB is not found\n");
		}
		if (load_dtb && !is_sdmmc) {
			if (is_nand) {
				np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash/partitions");
			} else {
				np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
			}

			if (np >= 0) {
				u32 npart = 0;
				fdt_get_property_u_32_index(np, "part-num", 0, &npart);
				for (unsigned i = 1; i <= npart; i++) {
					char strbuf[512];
					const char *partname;
					u32 t_start = 0;
					memset(strbuf, 0, sizeof(strbuf));
					snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
					fdt_get_property_u_32_index(np, strbuf, 0, &t_start);
					snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
					if (fdt_get_property_string_index(np, strbuf, 0, &partname))
						continue;
					if (strcmp(partname, "dtb"))
						continue;
					dtb_from = 0xafc00000 | t_start;
					break;
				}
			}
			np = fdt_get_node_offset_by_path("/hcrtos/memory-mapping/bootmem");
			if (np >= 0) {
				fdt_get_property_u_32_index(np, "reg", 0, &dtb_to);
				if (dtb_to != 0) {
					dtb_to |= 0xa0000000;
					dtb_to -= dtb_size;
				}
			}
			if (dtb_from == 0 || dtb_size == 0 || dtb_to == 0)
				die("bootloader needs a separate DTB but the DTB is not found\n");
		}

		if (!is_sdmmc) {
			if (is_nand) {
				np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
			} else {
				np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
			}
			if (np >= 0) {
				uint32_t wire = 0;
				fdt_get_property_u_32_index(np, "spi-rx-bus-width", 0, &wire);
				if (wire == 1 || wire == 2) {
					spi_wire = wire;
				} else if (wire == 4) {
					spi_wire = 2;
				}
			}
		}

		np = fdt_get_node_offset_by_path("/hcrtos/scpu");
		if (np >= 0) {
			uint32_t val = 0;
			fdt_get_property_u_32_index(np, "clock", 0, &val);
			if (val != 0)
				scpu_clk = (unsigned short)val;
			val = 0;
			fdt_get_property_u_32_index(np, "scpu-dig-pll-clk", 0, &val);
			if (val != 0)
				scpu_dpll_clk = (unsigned short)val;
		}
		np = fdt_get_node_offset_by_path("/hcrtos/mcpu");
		if (np >= 0) {
			uint32_t val = 0;
			fdt_get_property_u_32_index(np, "clock", 0, &val);
			if (val != 0)
				mcpu_clk = (unsigned short)val;
			val = 0;
			fdt_get_property_u_32_index(np, "mcpu-dig-pll-clk", 0, &val);
			if (val != 0)
				mcpu_dpll_clk = (unsigned short)val;
		}
		free(dtb);
	}

	if (is_nand) {
		buf = malloc((unsigned int)sb.st_size + nand_pagesize);
		memset(buf, 0, (unsigned int)sb.st_size + nand_pagesize);
		*(uint32_t *)(buf + 0x0) = 0xaeeaaeea;
		*(uint32_t *)(buf + 0x4) = 0xaeeaaeea;
		ptr = buf + nand_pagesize;
		from += nand_pagesize;
	} else {
		buf = malloc((unsigned int)sb.st_size);
		memset(buf, 0, (unsigned int)sb.st_size);
		ptr = buf;
	}

	fseek(fpin, 0, SEEK_SET);
	if ((ret = fread(ptr, 1, (unsigned int)sb.st_size, fpin)) < 0) {
		printf("read input %s failed\n", input);
		free(buf);
		return -1;
	}

	magic = *(uint32_t *)(ptr + 0x10);
	if (magic == 0x5a5aa5a5) {
		if (is_nand) {
			*(uint32_t *)(ptr + 0x10 + 0x4) = 0xeaaeeaae;
			*(uint32_t *)(ptr + 0x10 + 0x8) = nand_pagesize;
			*(uint32_t *)(ptr + 0x10 + 0xc) = nand_erasesize;
		} else if (is_sdmmc) {
			*(uint32_t *)(ptr + 0x10 + 0x4) = 0xe00ce00c;
			from -= 0x4000;
			to -= 0x4000;
			size += 0x4000;
		}
		*(uint32_t *)(ptr + 0x0) = dtb_size;
		*(uint32_t *)(ptr + 0x4) = dtb_to;
		*(uint32_t *)(ptr + 0x8) = dtb_from;
		*(uint32_t *)(ptr + 0x20 + 0x0) = size;
		*(uint32_t *)(ptr + 0x20 + 0x4) = to;
		*(uint32_t *)(ptr + 0x20 + 0x8) = from;
		*(uint32_t *)(ptr + 0x20 + 0xc) = entry;
		*(unsigned char *)(ptr + 0x30 + 0x0) = hcprogrammer_usb0_en;
		*(unsigned char *)(ptr + 0x30 + 0x1) = hcprogrammer_usb1_en;
		*(unsigned int *)(ptr + 0x30 + 0x4) = hcprogrammer_irq_timeout;
		*(unsigned int *)(ptr + 0x30 + 0x8) = hcprogrammer_sync_timeout;
		*(unsigned char *)(ptr + 0x30 + 0xc) = hcprogrammer_support_winusb;
		*(unsigned int *)(ptr + 0x40 + 0x0) = spi_wire;
		*(unsigned int *)(ptr + 0x40 + 0x4) = spi_sclk;
		*(unsigned short *)(ptr + 0x40 + 0x8) = scpu_clk; /* scpu clock selection */
		*(unsigned short *)(ptr + 0x40 + 0xa) = scpu_dpll_clk; /* scpu dpll clock (MHz) */
		*(unsigned short *)(ptr + 0x40 + 0xc) = mcpu_clk; /* mcpu clock selection */
		*(unsigned short *)(ptr + 0x40 + 0xe) = mcpu_dpll_clk; /* mcpu dpll clock (MHz) */
		if (firmware_version) {
			*(unsigned int *)(ptr + 0x50 + 0x0) = firmware_version;
			memcpy((void *)(ptr + 0x60 + 0x0), product_id, sizeof(product_id));
		}
	}

	printf("Pre-boot fixup hcprogrammer for USB0     [%s]\n", hcprogrammer_usb0_en ? "Enabled" : "Disabled");
	printf("Pre-boot fixup hcprogrammer for USB1     [%s]\n", hcprogrammer_usb1_en ? "Enabled" : "Disabled");
	printf("Pre-boot fixup hcprogrammer for driver   [%s]\n", hcprogrammer_support_winusb ? "WinUSB" : "HiChipUSB");
	printf("Pre-boot fixup hcprogrammer sync timeout [%d]\n", hcprogrammer_sync_timeout);
	printf("Pre-boot fixup hcprogrammer irq timeout  [%d]\n", hcprogrammer_irq_timeout);
	printf("Pre-boot fixup boot mode                 [%s]\n", is_nand ? "SPI Nand" : (is_sdmmc ? "SDMMC" : "SPI Nor"));
	printf("Pre-boot fixup bootloader size           [%d]\n", size);
	printf("Pre-boot fixup bootloader flash address  [0x%08x]\n", from);
	printf("Pre-boot fixup bootloader dram address   [0x%08x]\n", to);
	printf("Pre-boot fixup bootloader entry address  [0x%08x]\n", entry);
	printf("Pre-boot fixup spi rx bus width          [%d]\n", spi_wire);
	printf("Pre-boot fixup spi clock                 [%d]\n", spi_sclk);
	printf("Pre-boot fixup scpu clock selection      [%d]\n", scpu_clk);
	printf("Pre-boot fixup scpu dpll clock           [%d]\n", scpu_dpll_clk);
	printf("Pre-boot fixup mcpu clock selection      [%d]\n", mcpu_clk);
	printf("Pre-boot fixup mcpu dpll clock           [%d]\n", mcpu_dpll_clk);
	printf("Pre-boot fixup dtb size                  [0x%08x]\n", dtb_size);
	printf("Pre-boot fixup dtb from                  [0x%08x]\n", dtb_from);
	printf("Pre-boot fixup dtb to                    [0x%08x]\n", dtb_to);
	if (firmware_version) {
		if (firmware_version & 0xff000000)
			printf("Pre-boot fixup firmware version          [%u]\n", firmware_version);
		else
			printf("Pre-boot fixup firmware version          v%u.%u.%u\n", firmware_version >> 16, (firmware_version >> 8) & 0xff, firmware_version & 0xff);
		if (strlen(product_id) == 0)
			strncpy(product_id, "none", sizeof(product_id));
		printf("Pre-boot fixup product_id                [%s]\n", product_id);
	}


	fseek(fpout, 0, SEEK_SET);
	if (is_nand)
		fwrite(buf, 1, (unsigned int)sb.st_size + nand_pagesize, fpout);
	else
		fwrite(buf, 1, (unsigned int)sb.st_size, fpout);

	if (!output) {
		fclose(fpout);
	} else {
		fclose(fpin);
		fclose(fpout);
	}

	free(buf);
	return 0;
}

