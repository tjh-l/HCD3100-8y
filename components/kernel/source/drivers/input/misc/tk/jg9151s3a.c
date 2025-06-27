#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <hcuapi/gpio.h>
#include <hcuapi/i2c-master.h>

#define MAX_TK_KEY_NUM	10
#define TK_KEY_MASK	0x03FF
#define TK_RUNNING	1
#define TK_STOP		0

typedef struct {
	uint32_t key_val;
	uint32_t key_code;
} hc_tkkey_map_s;

struct tk_jg9151s3a {
	const char *i2c_devpath;
	const char *adc_devpath;
	struct input_dev		*input;
	unsigned int irq;
	unsigned int addr;
	struct timer_list		tk_timer;	
	unsigned short 			key_data;
	int				keymap_len;
	hc_tkkey_map_s			*key_map;
	uint16_t 			active_key;
	struct mutex 			operation_mutex;
	bool				status;
};

int tk_i2c_read(struct tk_jg9151s3a *ts, unsigned char addr,
		 unsigned char *readbuf, int readlen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)ts->addr,
			.flags = I2C_M_RD,
			.length = readlen,
			.buffer = readbuf,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(ts->i2c_devpath, O_RDWR);

	while (retry--) {
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if (ret < 0) {
			printf("error.retry = %d\n", retry);
			continue;
		}
		break;
	}

	close(fd);

	return ret;
}

static void tk_timer_callback(struct timer_list *param)
{
	struct tk_jg9151s3a *tk = from_timer(tk, param, tk_timer);
	int i, j, ret;
	uint16_t tmp;
	static uint16_t last_active;

	if (tk->status == TK_STOP)
		goto out;

	ret = tk_i2c_read(tk, 0, (unsigned char *)&tk->key_data, 2);
	if (ret < 0) {
		printf("tk jg9151s3a read data fail!\n");
		goto out;
	}

	if ((last_active != tk->active_key) || ((tk->key_data & TK_KEY_MASK) == 0x00)) {
		for (j = 0; j < tk->keymap_len; j++) {
			if (tk->key_map[j].key_val & last_active) {
				input_report_key(tk->input, tk->key_map[j].key_code, 0);
				input_sync(tk->input);
				tk->active_key = 0;
			}
		}
	}

	last_active = tk->active_key;

	for (i = 0; i < MAX_TK_KEY_NUM; i++) {
		tmp = (1 << i);
		if (tmp & tk->key_data) {
			for (j = 0; j < tk->keymap_len; j++) {
				if (tk->key_map[j].key_val & tmp) {
					if (last_active == 0) {
						input_report_key(tk->input, tk->key_map[j].key_code, 1);
						input_sync(tk->input);
					}
					tk->active_key |= tmp;
				}
			}
		}
	}

out:
	tk->key_data = 0;
	mod_timer(&tk->tk_timer, jiffies + usecs_to_jiffies(25 * 1000));
}

static int hc_jg9151s3a_open(struct input_dev *dev)
{
	struct tk_jg9151s3a *tk = input_get_drvdata(dev);

	mutex_lock(&(tk->operation_mutex));
	tk->status = TK_RUNNING;
	mutex_unlock(&(tk->operation_mutex));

	return 0;
}

static void hc_jg9151s3a_close(struct input_dev *dev)
{
	struct tk_jg9151s3a *tk = input_get_drvdata(dev);

	mutex_lock(&(tk->operation_mutex));
	tk->status = TK_STOP;
	mutex_unlock(&(tk->operation_mutex));

	return;
}

static int hc_tk_jg9151s3a_probe(char *node)
{
	int np;
	int ret = -EINVAL;
	struct tk_jg9151s3a *tk = kzalloc(sizeof(struct tk_jg9151s3a), GFP_KERNEL);

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		printf("No Find touch key \n");
		return -ENODEV;
	}

	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&tk->addr))
		return -1;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &tk->i2c_devpath))
		return -1;

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &tk->keymap_len);
	if (ret < 0) {
		printf("Touch Key No Find key-num\n");
		return -ENODEV;
	}

	tk->key_map = (hc_tkkey_map_s *)malloc(sizeof(hc_tkkey_map_s) * tk->keymap_len);
	ret = fdt_get_property_u_32_array(np, "key-map", (u32 *)tk->key_map, tk->keymap_len * 2);
	if (ret < 0) {
		printf("Touch Key No Find key-map\n");
		return -ENODEV;
	}

	mutex_init(&(tk->operation_mutex));

	tk->input= input_allocate_device();
	if(!tk->input)
		goto err;

	tk->input->name = "touch_key";
	__set_bit(EV_KEY, tk->input->evbit);
	__set_bit(EV_REP, tk->input->evbit);

	for (int i = 0; i < tk->keymap_len; i++)
		set_bit(tk->key_map[i].key_code, tk->input->keybit);

	tk->input->open = hc_jg9151s3a_open;
	tk->input->close = hc_jg9151s3a_close;

	input_set_drvdata(tk->input, tk);

	ret = input_register_device(tk->input);
	if (ret < 0)
		goto err;

        timer_setup(&tk->tk_timer, tk_timer_callback, 0);

	tk_timer_callback(&tk->tk_timer);

	return ret;
err:
	kfree(tk);

	return -1;

}

static int hc_tk_jg9151s3a_init(void)
{
	int ret = 0;

	ret = hc_tk_jg9151s3a_probe("/hcrtos/jg9151s3a@0");

	return ret;
}

module_driver(tk_jg9151s3a, hc_tk_jg9151s3a_init, NULL, 2)
