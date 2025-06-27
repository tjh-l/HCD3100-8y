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

#include <kernel/vfs.h>

#define EC1106S_FIFO_LEN 16
#define EC1106S_EVENT_LEFT 1
#define EC1106S_EVENT_RIGHT 2

struct ec1106s_dev {
	int input_a;
	int input_b;
	int32_t counter;
	struct input_dev *input;
	DECLARE_KFIFO(kfifo, uint8_t, EC1106S_FIFO_LEN);
	struct completion completion;
	TaskHandle_t ec1106s_kthread;
};

static void ec1106s_kthread(void *pvParameters)
{
	struct ec1106s_dev *ec1106s = (struct ec1106s_dev *)pvParameters;
	uint8_t ev;

	while (1) {
		wait_for_completion(&ec1106s->completion);

		if (kfifo_out(&ec1106s->kfifo, &ev, 1)) {
			if (ev == EC1106S_EVENT_LEFT) {
				input_report_key(ec1106s->input, KEY_LEFT, 1);
				input_sync(ec1106s->input);
				input_report_key(ec1106s->input, KEY_LEFT, 0);
				input_sync(ec1106s->input);
			} else if (ev == EC1106S_EVENT_RIGHT) {
				input_report_key(ec1106s->input, KEY_RIGHT, 1);
				input_sync(ec1106s->input);
				input_report_key(ec1106s->input, KEY_RIGHT, 0);
				input_sync(ec1106s->input);
			}
		}
	}
}

static void ec1106s_a_irq(uint32_t param)
{
	struct ec1106s_dev *ec1106s = (struct ec1106s_dev *)param;
	uint8_t ev;
	static int level = 1;

	int input_a = gpio_get_input(ec1106s->input_a);
	int input_b = gpio_get_input(ec1106s->input_b);

	if (input_a == level) {
		return;
	} else {
		level = input_a;
	}

	if (input_a) {
		return;
	}

	if (input_b) {
		ec1106s->counter++;
		ev = EC1106S_EVENT_LEFT;
	} else {
		ec1106s->counter--;
		ev = EC1106S_EVENT_RIGHT;
	}

	kfifo_put(&ec1106s->kfifo, ev);
	complete(&ec1106s->completion);
}

static ssize_t ec1106s_read(struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct ec1106s_dev *ec1106s = inode->i_private;

	if (buflen < sizeof(ec1106s->counter))
		return -EINVAL;

	*(int32_t *)buffer = ec1106s->counter;

	return sizeof(ec1106s->counter);
}

static const struct file_operations ec1106s_fops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = ec1106s_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = NULL, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int ec1106s_probe(const char *node)
{
	int np;
	int ret = 0;
	struct ec1106s_dev *ec1106s;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	ec1106s = kzalloc(sizeof(struct ec1106s_dev), GFP_KERNEL);
	if (!ec1106s) {
		printf("No memory!\n");
		return -ENOMEM;
	}

	ec1106s->input_a = PINPAD_INVALID;
	ec1106s->input_b = PINPAD_INVALID;

	INIT_KFIFO(ec1106s->kfifo);
	init_completion(&ec1106s->completion);

	if (fdt_get_property_u_32_index(np, "gpio-input-a", 0, (u32 *)&ec1106s->input_a)) {
		printf("No find gpio-input-a\n");
		goto err;
	}

	if (fdt_get_property_u_32_index(np, "gpio-input-b", 0, (u32 *)&ec1106s->input_b)) {
		printf("No find gpio-input-b\n");
		goto err;
	}

	gpio_configure(ec1106s->input_b, GPIO_DIR_INPUT);
	gpio_configure(ec1106s->input_a, GPIO_DIR_INPUT | GPIO_IRQ_RISING | GPIO_IRQ_RISING);
	gpio_irq_disable(ec1106s->input_a);
	ret = gpio_irq_request(ec1106s->input_a, ec1106s_a_irq, (uint32_t)ec1106s);
	if (ret < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", ret);
		goto err;
	}

	ec1106s->input = input_allocate_device();
	if(!ec1106s->input)
		goto err;

	ec1106s->input->name = "ec1106s";
	__set_bit(EV_KEY, 	ec1106s->input->evbit);
	__set_bit(KEY_LEFT, 	ec1106s->input->keybit);
	__set_bit(KEY_RIGHT, 	ec1106s->input->keybit);

	ret = input_register_device(ec1106s->input);
	if (ret) {
		printf("ec1106s_probe: failed to register input device\n");
		goto err;
	}

	xTaskCreate(ec1106s_kthread, (const char *)"ec1106s_kthread", configTASK_STACK_DEPTH, ec1106s,
		    portPRI_TASK_NORMAL, &ec1106s->ec1106s_kthread);


	ret = register_driver("/dev/ec1106s", &ec1106s_fops, 0666, ec1106s);
	if (ret < 0) {
		printf("register /dev/ec1106s error!!!\n");
		goto err;
	}

	gpio_irq_enable(ec1106s->input_a);
err:
	return ret;
}

static int hc_ec1106s_init(void)
{
	int ret = 0;

	ret = ec1106s_probe("/hcrtos/ec1106s@0");

	return ret;
}

module_driver(hc_ec1106s_driver, hc_ec1106s_init, NULL, 1)
