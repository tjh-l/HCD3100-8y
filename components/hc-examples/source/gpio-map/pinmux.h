#ifndef __PINMUX_H_H_LINUX_
#define __PINMUX_H_H_LINUX_

#ifndef __HCRTOS__
	typedef unsigned char pinmux_pinset_t;
#endif

void pinmux_init(void *reg_base, uint32_t phy_reg_base);
void pinmux_deinit(void);
int pinmux_configure(pinpad_e padctl, pinmux_pinset_t muset);

#endif
