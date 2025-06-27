#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <hcuapi/pinpad.h>
#include "io.h"
#include "pinmux.h"

#ifndef CONFIG_SOC_HC16XX
#define CONFIG_SOC_HC16XX 1
#endif

#define PINMUXL 	0x188004a0
#define PINMUXB 	0x188004e0
#define PINMUXR 	0x18800520
#define PINMUXT 	0x18800560
#define DRIVER_CAP 	0x18800184

static void *pinmuxreg[4];
static void *driver_cap;
void pinmux_init(void *reg_base, uint32_t phy_reg_base)
{
	pinmuxreg[0] = reg_base + (PINMUXL - phy_reg_base);
	pinmuxreg[1] = reg_base + (PINMUXB - phy_reg_base);
	pinmuxreg[2] = reg_base + (PINMUXR - phy_reg_base);
	pinmuxreg[3] = reg_base + (PINMUXT - phy_reg_base);
	driver_cap = reg_base + (DRIVER_CAP - phy_reg_base);
}

void pinmux_deinit(void)
{
	memset(&pinmuxreg[0], 0, sizeof(pinmuxreg));
	driver_cap = NULL;
}

int pinmux_configure(pinpad_e padctl, pinmux_pinset_t muset)
{
#ifdef CONFIG_SOC_HC16XX
	if (padctl >= PINPAD_T00 && padctl <= PINPAD_T05 && muset == 0) {
		muset |= (0x7 << 3); /* enhance driver capability */
		REG32_SET_BIT(driver_cap, BIT24);
	}
#endif
	if (padctl < 32)
		REG8_WRITE((uint32_t)pinmuxreg[0] + padctl, muset);
	else if (padctl < 64)
		REG8_WRITE((uint32_t)pinmuxreg[1] + padctl - 32, muset);
	else if (padctl < 96)
		REG8_WRITE((uint32_t)pinmuxreg[2] + padctl - 64, muset);
	else if (padctl < 128)
		REG8_WRITE((uint32_t)pinmuxreg[3] + padctl - 96, muset);
	else
		return -1;

	return 0;
}
