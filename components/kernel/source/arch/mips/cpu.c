#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/io.h>
#include <kernel/irqflags.h>
#include <kernel/lib/console.h>
#include <hcuapi/standby.h>
#include <kernel/ld.h>

int reset(void)
{
	void *wdt_addr = (void *)&WDT0;

	arch_local_irq_disable();
	REG32_WRITE(wdt_addr, 0xfffffffa);
	REG32_WRITE(wdt_addr + 4, 0x26);
	asm volatile(".word 0x1000ffff; nop; nop;"); /* Wait for reboot */
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-ush]\n", prog);
	puts("  -u --upgrade  reboot enter usb upgrade\n"
	     "  -s --slot     reboot enter firmware slot [p|primary, s|secondary]\n"
	     "  -l --logo     reboot with boot feature showlogo [on, off]\n"
	     "  -f --feature  reboot with feature, 0: undef, 1: logo on, 2: logo off, 3: osdlogo on, 4: osdlogo off\n"
	     "  -h --help     print help usage\n");
}

static int do_reset(int argc, char **argv)
{
	int nr;
	int feature;
	unsigned long slot = REG8_READ(0xb8818a71);

	nr = STANDBY_BOOTUP_SLOT_NR(slot);
	feature = STANDBY_BOOTUP_SLOT_FEATURE(slot);

	while (1) {
		static const struct option lopts[] = {
			{ "upgrade", 1, 0, 'u' },
			{ "slot",    1, 0, 's' },
			{ "logo",    1, 0, 'l' },
			{ "feature", 1, 0, 'f' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "us:l:f:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'u':
			REG16_WRITE(0xb8818a00, 0x5991);
			break;
		case 's':
			if (!strcasecmp(optarg, "p") || !strcasecmp(optarg, "primary"))
				nr = STANDBY_BOOTUP_SLOT_NR_PRIMARY;
			else if (!strcasecmp(optarg, "s") || !strcasecmp(optarg, "secondary")) {
				nr = STANDBY_BOOTUP_SLOT_NR_SECONDARY;
			}
			break;
		case 'l':
			if (!strcasecmp(optarg, "on"))
				feature = STANDBY_BOOTUP_SLOT_FEATURE_LOGO_ON;
			else if (!strcasecmp(optarg, "off"))
				feature = STANDBY_BOOTUP_SLOT_FEATURE_LOGO_OFF;
			break;
		case 'f':
			feature = strtoll(optarg, NULL, 0);
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 0;
		}
	}

	slot = STANDBY_BOOTUP_SLOT(nr, feature);
	REG8_WRITE(0xb8818a71, slot);

	reset();
	return 0;
}

CONSOLE_CMD(reset, NULL, do_reset, CONSOLE_CMD_MODE_SELF, "reset system")
