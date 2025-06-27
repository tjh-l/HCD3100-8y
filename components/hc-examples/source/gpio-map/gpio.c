#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <hcuapi/pinpad.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include "io.h"
#include "hc_gpio.h"
#include <stdlib.h>
#include "gpio.h"

#define REGISTER_BASE 	0x18800000
#define REGISTER_SIZE 	(1028*8)
#define GPIOLCTRL       0x18800044
#define GPIOBCTRL       0x188000c4
#define GPIORCTRL       0x188000e4
#define GPIOTCTRL       0x18800344

#if 1
#define GPIO_DBG(...)
#else
#define GPIO_DBG printf
#endif

static void *reg_base;
static void * ctrlreg[4];

int gpio_init(void)
{
	int fd = 0;
	if(reg_base) {
		return 0;
	}

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		GPIO_DBG("%s:%d:%s\n", __func__, __LINE__, strerror(errno));
		return -EIO;
	}

	reg_base = (void *)mmap(NULL, REGISTER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, REGISTER_BASE);
	if (reg_base == MAP_FAILED) {
		reg_base = NULL;
		GPIO_DBG("%s:%d:%s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	ctrlreg[0] = reg_base + (GPIOLCTRL - REGISTER_BASE);
	ctrlreg[1] = reg_base + (GPIOBCTRL - REGISTER_BASE);
	ctrlreg[2] = reg_base + (GPIORCTRL - REGISTER_BASE);
	ctrlreg[3] = reg_base + (GPIOTCTRL - REGISTER_BASE);

	pinmux_init(reg_base, REGISTER_BASE);
	return 0;
}

void gpio_deinit(void)
{
	if(reg_base) {
		munmap(reg_base, REGISTER_SIZE);
		reg_base = NULL;
	}
	pinmux_deinit();
}

static void gpio_config_setin(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + DIR_REG;

	REG32_CLR_BIT(reg, bit);

	return ;
}

static void gpio_config_setout(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + DIR_REG;

	REG32_SET_BIT(reg, bit);

	return ;
}

static void gpio_set_output_high(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + OUTPUT_VAL_REG;

	REG32_SET_BIT(reg, bit);

	return ;
}

static void gpio_set_output_low(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + OUTPUT_VAL_REG;

	REG32_CLR_BIT(reg, bit);

	return ;
}

/******************************************************************************
 * Public Function
 *****************************************************************************/
void gpio_set_output(pinpad_e padctl, bool val)
{
	if (padctl >= PINPAD_MAX)
		return;

	if (val)
		gpio_set_output_high(padctl);
	else
		gpio_set_output_low(padctl);
	return ;
}

int gpio_get_input(pinpad_e padctl)
{
	uint32_t bit = padctl % 32;
	void *reg = ctrlreg[padctl / 32] + INPUT_ST_REG;

	if (padctl >= PINPAD_MAX)
		return -1;

	return ((REG32_READ(reg) >> bit) & 0x1);
}

int gpio_configure(pinpad_e padctl, gpio_pinset_t pinset)
{
	if (padctl >= PINPAD_MAX)
		return -1;

	pinmux_configure(padctl, 0);
	switch (pinset & GPIO_DIR_MUSK) {
	case GPIO_DIR_INPUT:
		gpio_config_setin(padctl);
		break;
	case GPIO_DIR_OUTPUT:
		gpio_config_setout(padctl);
		break;
	default:
		break;
	}
	return 0;
}

int __attribute__((weak)) main(int argc, char *argv[])
{
	int pinpad = -1;
	int val = 0;
	if(argc != 3){
		printf("Usage: %s <gpio number> <output val>\n", argv[0]);
		return -1;
	}
	pinpad = atoi(argv[1]);
	val = atoi(argv[2]);
	printf("gpio number: %d, val: %d\n", pinpad, val);
	gpio_init();
	if(gpio_configure(pinpad, GPIO_DIR_OUTPUT) != 0)
		printf("set gpio output fail.\n");
	gpio_set_output(pinpad, val);
	gpio_deinit();
	return 0;
}
