#include <linux/interrupt.h>
#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <hcuapi/gpio.h>
#include <kernel/delay.h>
#include <kernel/io.h>
#include <kernel/completion.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <linux/kfifo.h>

#define MODEL_NAME "sal61955m"
#define sal61955m_gpioA "gpio-A"
#define sal61955m_gpioB "gpio-B"
#define sal61955m_gpioKEY "gpio-KEY"
#define SAL61955M_MIN_LEFT_DURATION 800ULL /* in usec */

#define SAL61955M_KEY_FIFO_LEN 16
#define SAL61955M_KEY_EVENT_LEFT 1
#define SAL61955M_KEY_EVENT_RIGHT 2

struct sal61955m_data {
	struct input_dev *input_dev;
	u32 gpiodA;
	u32 gpiodB;
	u32 gpiodKey;

	DECLARE_KFIFO(kfifo, uint8_t, SAL61955M_KEY_FIFO_LEN);
	struct completion completion;
	TaskHandle_t key_thread;
};

#define IER_REG				0x00
#define RIS_IER_REG			0x04
#define FALL_IER_REG 			0x08
#define INPUT_ST_REG			0x0c
#define OUTPUT_VAL_REG			0x10
#define DIR_REG				0x14
#define ISR_REG				0x18
static void *ctrlreg[4] = {
	(void *)&GPIOLCTRL,
	(void *)&GPIOBCTRL,
	(void *)&GPIORCTRL,
	(void *)&GPIOTCTRL,
};

static void key_kthread(void *pvParameters)
{
	struct sal61955m_data *sal61955m =
		(struct sal61955m_data *)pvParameters;
	uint8_t ev;

	msleep(2 * 1000);
	gpio_irq_enable(sal61955m->gpiodB);
	gpio_irq_enable(sal61955m->gpiodA);

	while (1) {
		wait_for_completion(&sal61955m->completion);

		if (kfifo_out(&sal61955m->kfifo, &ev, 1)) {
			if (ev == SAL61955M_KEY_EVENT_LEFT) {
				input_report_key(sal61955m->input_dev, KEY_LEFT,
						 1);
				input_sync(sal61955m->input_dev);
				input_report_key(sal61955m->input_dev, KEY_LEFT,
						 0);
				input_sync(sal61955m->input_dev);
			} else if (ev == SAL61955M_KEY_EVENT_RIGHT) {
				input_report_key(sal61955m->input_dev,
						 KEY_RIGHT, 1);
				input_sync(sal61955m->input_dev);
				input_report_key(sal61955m->input_dev,
						 KEY_RIGHT, 0);
				input_sync(sal61955m->input_dev);
			}
		}
	}
}

static void sal61955m_A_int(uint32_t param)
{
	return;
}

static void sal61955m_B_int(uint32_t param)
{
	struct sal61955m_data *sal61955m = (struct sal61955m_data *)param;
	uint8_t key_event = 0;
	uint64_t btime = 0;
	uint64_t atime = 0;
	uint32_t bit;
	void *reg;
	pinpad_e padctl_A;

	padctl_A = sal61955m->gpiodA;
	bit = BIT(padctl_A % 32);
	reg = ctrlreg[padctl_A / 32] + ISR_REG;

	btime = uxPortTimerHrTickGet();
	while (1) {
		atime = uxPortTimerHrTickGet();
		if (REG32_GET_BIT(reg, bit))
			break;

		if ((atime - btime) > SAL61955M_MIN_LEFT_DURATION)
			break;
	}

	if ((atime - btime) > SAL61955M_MIN_LEFT_DURATION)
		key_event = SAL61955M_KEY_EVENT_LEFT;
	else
		key_event = SAL61955M_KEY_EVENT_RIGHT;

	kfifo_put(&sal61955m->kfifo, key_event);
	complete(&sal61955m->completion);
}

static void sal61955m_key_int(uint32_t param)
{
	gpio_pinset_t irq_falling = GPIO_DIR_INPUT | GPIO_IRQ_FALLING;
	gpio_pinset_t irq_rising = GPIO_DIR_INPUT | GPIO_IRQ_RISING;
	static char keydown = 1;
	struct sal61955m_data *sal61955m = (struct sal61955m_data *)param;

	if (keydown) {
		gpio_config_irq(sal61955m->gpiodKey, irq_rising);
		input_report_key(sal61955m->input_dev, KEY_ENTER, 1);
		input_sync(sal61955m->input_dev);
	} else {
		gpio_config_irq(sal61955m->gpiodKey, irq_falling);
		input_report_key(sal61955m->input_dev, KEY_ENTER, 0);
		input_sync(sal61955m->input_dev);
	}

	keydown = !keydown;
}

static int sal61955m_probe(const char *node)
{
	int err = 0;
	int np;
	gpio_pinset_t irq_pinset = GPIO_DIR_INPUT | GPIO_IRQ_FALLING;
	struct input_dev *input_dev;
	struct sal61955m_data *sal61955m = NULL;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	sal61955m = malloc(sizeof(struct sal61955m_data));

	if (!sal61955m) {
		printf("no memory!\n");
		return -ENOMEM;
	}
	memset(sal61955m, 0, sizeof(struct sal61955m_data));
	INIT_KFIFO(sal61955m->kfifo);
	init_completion(&sal61955m->completion);

	if (fdt_get_property_u_32_index(np, "ena-gpios", 0,
					(u32 *)&sal61955m->gpiodA)) {
		sal61955m->gpiodA = PINPAD_INVALID;
	}
	if (fdt_get_property_u_32_index(np, "enb-gpios", 0,
					(u32 *)&sal61955m->gpiodB)) {
		sal61955m->gpiodB = PINPAD_INVALID;
	}
	if (fdt_get_property_u_32_index(np, "key-gpios", 0,
					(u32 *)&sal61955m->gpiodKey)) {
		sal61955m->gpiodKey = PINPAD_INVALID;
	}
	printf("%s gpioA:%d gpioB:%d gpiokey:%d\n", __func__, sal61955m->gpiodA,
	       sal61955m->gpiodB, sal61955m->gpiodKey);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		printf("failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	input_dev->name = "sal61955m";
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(KEY_ENTER, input_dev->keybit);
	__set_bit(KEY_LEFT, input_dev->keybit);
	__set_bit(KEY_RIGHT, input_dev->keybit);

	err = input_register_device(input_dev);
	if (err) {
		printf("sal61955m_probe: failed to register input device\n");
		goto exit_input_register_device_failed;
	}

	sal61955m->input_dev = input_dev;
	input_set_drvdata(sal61955m->input_dev, sal61955m);

	gpio_configure(sal61955m->gpiodB, irq_pinset);
	err = gpio_irq_request(sal61955m->gpiodB, sal61955m_B_int,
			       (uint32_t)sal61955m);
	if (err < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", err);
	}
	gpio_irq_disable(sal61955m->gpiodB);

	gpio_configure(sal61955m->gpiodA, irq_pinset);
	err = gpio_irq_request(sal61955m->gpiodA, sal61955m_A_int,
			       (uint32_t)sal61955m);
	if (err < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", err);
	}
	gpio_irq_disable(sal61955m->gpiodA);

	gpio_configure(sal61955m->gpiodKey, irq_pinset);
	err = gpio_irq_request(sal61955m->gpiodKey, sal61955m_key_int,
			       (uint32_t)sal61955m);
	if (err < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", err);
	}

	xTaskCreate(key_kthread, (const char *)"rotary_key_kthread",
		    configTASK_STACK_DEPTH, sal61955m, portPRI_TASK_NORMAL,
		    &sal61955m->key_thread);

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free(sal61955m);

	return err;
}

static int sal61955m_init(void)
{
	int rc = 0;

	rc |= sal61955m_probe("/hcrtos/rotary_key");

	return rc;
}

module_driver(hc_sal61955m_drv, sal61955m_init, NULL, 1)
