#include <generated/br2_autoconf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <kernel/io.h>
#include <hcuapi/gpio.h>
#include <hcuapi/standby.h>
#include <mtdload.h>
#include <bootm.h>

#define max(a, b)                                                              \
	({                                                                     \
		typeof(a) _a = a;                                              \
		typeof(b) _b = b;                                              \
		_a > _b ? _a : _b;                                             \
	})

extern void stdio_initalize(void);

const char *fdt_get_sysmem_path(void)
{
	return "/hcrtos/memory-mapping/bootmem";
}

static int get_partinfo(const char *name, unsigned char *dev_type, u32 *start, u32 *size)
{
	int np = -1;
	u32 npart = 0;
	u32 i = 1;
	int rc;
	const char *partname;
	const char *status = NULL;
	char strbuf[512];

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

		for (i = 1; i <= npart; i++) {
			*start = 0;
			*size = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, start);
			fdt_get_property_u_32_index(np, strbuf, 1, size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			if (strncmp(partname, name, max(strlen(partname), strlen(name))))
				continue;

			*dev_type = IH_DEVT_SPINOR;
			return 0;
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

		if (npart == 0)
			break;

		for (i = 1; i <= npart; i++) {
			*start = 0;
			*size = 0;

			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
			fdt_get_property_u_32_index(np, strbuf, 0, start);
			fdt_get_property_u_32_index(np, strbuf, 1, size);
			snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
			if (fdt_get_property_string_index(np, strbuf, 0, &partname))
				continue;
			if (strncmp(partname, name, max(strlen(partname), strlen(name))))
				continue;

			*dev_type = IH_DEVT_SPINAND;
			return 0;
		}
	} while (0);

	return -1;
}

int close_watchdog(void)
{
	REG32_WRITE(0xb8818500 + 4, 0);
}

static void set_gpio_def_st(void)
{
	int np, num_pins, i, pin, pin_val;
	bool val;

	np = fdt_node_probe_by_path("/hcrtos/gpio-out-def");
	if (np < 0)
		return;

	num_pins = 0;
	if (fdt_get_property_data_by_name(np, "gpio-group", &num_pins) == NULL)
		num_pins = 0;

	num_pins >>= 3;

	if (num_pins == 0)
		return;

	for (i = 0; i < num_pins; i++) {
		fdt_get_property_u_32_index(np, "gpio-group", i * 2, &pin);
		fdt_get_property_u_32_index(np, "gpio-group", i * 2 + 1, &pin_val);

		val = !pin_val;
		gpio_configure(pin, GPIO_DIR_OUTPUT);
		gpio_set_output(pin, val);
	}

	return;
}

static void boot_mode_setup(void)
{
	/*
	 * Using South Bridge Timer7 register to store the boot mode flag
	 * The South Bridge Timer7 register will only be cleared by power-off
	 */
	unsigned char flag = REG8_READ(0xb8818a70);
	if (flag != STANDBY_FLAG_COLD_BOOT && flag != STANDBY_FLAG_WARM_BOOT) {
		REG8_WRITE(0xb8818a70, STANDBY_FLAG_COLD_BOOT);
	} else {
		REG8_WRITE(0xb8818a70, STANDBY_FLAG_WARM_BOOT);
	}
}

static int show_meminfo(void)
{
#ifdef CONFIG_MINIBOOT_SHOWMEMINFO
	uint32_t size = 0, type = 0, frequency = 0, ic;

	type = REG32_GET_FIELD2(0xb8801000, 23, 1);
	size = REG32_GET_FIELD2(0xb8801000, 0, 3);
	frequency = REG32_GET_FIELD2(0xb8800070, 4, 3);

	type = type + 2;

	size = 16 << (size);

	switch (frequency) {
	case 0:
		frequency = 800;
		break;
	case 1:
		frequency = 1066;
		break;
	case 2:
		frequency = 1333;
		break;
	case 3:
		frequency = 1600;
		break;
	case 4:
		frequency = 600;
		break;
	default:
		break;
	}

	ic = REG8_READ(0xb8800003);
	if (REG16_GET_BIT(0xb880048a, BIT15)) {
		if (ic == 0x15)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 12;
		else if (ic == 0x16)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 24;
	}

	printf("\nDDR Info:\n");
	printf("\ttype:DDR%ld\n", type);
	printf("\tsize:%ldM\n", size);
	printf("\tfreq:%ldMHz\n\n", frequency);
#endif
	return 0;
}

int main(void)
{
	int ret;
	unsigned char dev_type;
	u32 start, size;
	unsigned long time_to_boot;

	time_to_boot = sys_time_from_boot();

	boot_mode_setup();

	close_watchdog();

	hc_clk_disable_all();

	set_gpio_def_st();

	stdio_initalize();

	printf("time to boot: %ld\r\n", time_to_boot);

	show_meminfo();

#if defined(CONFIG_MINIBOOT_HCRTOS)
	ret = get_partinfo("firmware", &dev_type, &start, &size);
	if (ret >= 0) {
		mtdloaduImage(dev_type, start, size);
		if (bootm(NULL, 0, 1, ((char *[]){ "bootm" })))
			reset();
	}
#elif defined(CONFIG_MINIBOOT_HCLINUX_SINGLECORE)
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	ret = get_partinfo("dtb", &dev_type, &start, &size);
	if (ret >= 0) {
		mtdloadraw(dtb, 0x10000, dev_type, start, size);
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_partinfo("linux", &dev_type, &start, &size);
	if (ret >= 0) {
		mtdloaduImage(dev_type, start, size);
		sprintf(loadaddr, "0x%08lx", image_load_addr);
		if (bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr })))
			upgrade_force();
	}
#endif

	/* Program should not run to here. */
	for (;;);
}
