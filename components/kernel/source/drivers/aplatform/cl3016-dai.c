#include <stdlib.h>
#include <string.h>
#include <kernel/soc/soc_common.h>
#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <nuttx/i2c/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <kernel/elog.h>
#include <kernel/ld.h>
#include <kernel/delay.h>
#include <kernel/lib/fdt_api.h>
#include <nuttx/wqueue.h>
#include <kernel/module.h>
#include <hcuapi/gpio.h>

struct dai_device {
	struct snd_capability cap;
	const char *i2c_dev;
	int fd;
};

static u32 i2c_slave_addr = 0;

/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K                                                    \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 | SND_PCM_RATE_96000)

#define SND_PCM_RATE_44_1K                                                     \
	(SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | SND_PCM_RATE_44100 | SND_PCM_RATE_88200)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats = (SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_S32_LE),
		},
};

static int cl3016_es_write_reg(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
{
	int ret = 0;
	uint8_t wdata[2] = {0,0};
	wdata[0] = reg_addr;
	wdata[1] = data;
	struct i2c_transfer_s xfer = {0};
	struct i2c_msg_s i2c_msg = {0};

	i2c_msg.addr = slave_addr;
	i2c_msg.flags = 0x0;
	i2c_msg.buffer = wdata;
	i2c_msg.length = 2;

	xfer.msgc = 1;
	xfer.msgv = &i2c_msg;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c write failed\n");
	}
	return ret;
}

static int cl3016_es_read_reg(int fd, uint8_t slave_addr, uint8_t *data)
{
	int ret = 0;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s i2c_msg[2] = {0};

	i2c_msg[0].addr = slave_addr;
	i2c_msg[0].buffer = &data[0];
	i2c_msg[0].flags = 0;
	i2c_msg[0].length = 1;

	i2c_msg[1].addr = slave_addr;
	i2c_msg[1].buffer = &data[1];
	i2c_msg[1].flags = 1;
	i2c_msg[1].length = 1;

	xfer.msgv = i2c_msg;
	xfer.msgc = 2;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c read failed\n");
	}

	return ret;
}

static int cl3016_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = 0x62;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return cl3016_es_write_reg(fd, slave_addr, reg_addr, pdata);
}

static int cl3016_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = 0x62;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return cl3016_es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int cl3016_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	int ret;
	/* cl3016 spec */
	/*if (chip_addr % 2 != 0) {
		chip_addr -= 1;
	}*/

	read_data[0] = chip_addr;
	ret = cl3016_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val || ret < 0) {
		printf("cl3016_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return ret;
}


static int cl3016_dai_hw_params(struct snd_soc_dai *dai, struct snd_pcm_params *params)
{

	return 0;
}

static int cl3016_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cl3016_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int cl3016_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cl3016_dai_hw_free(struct snd_soc_dai *dai)
{
	return 0;
}

static struct snd_soc_dai_driver cl3016_dai_driver = {
	.hw_params = cl3016_dai_hw_params,
	.trigger = cl3016_dai_trigger,
	.ioctl = cl3016_dai_ioctl,
	.get_capability = cl3016_dai_get_capability,
	.hw_free = cl3016_dai_hw_free,
};

static struct snd_soc_dai cl3016_for_i2si_dai = {
	.name = "cl3016-for-i2si-dai",
	.driver = &cl3016_dai_driver,
	.priv = &dai_dev,
};

int cl3016_dai_init(void)
{
	int np;
	const char *status;
	//printf("\n************cl3016_dai_init start ******************\n");
	dai_dev.i2c_dev = NULL;
	np = fdt_node_probe_by_path("/hcrtos/cl3016");
	if (np < 0) {
		return 0;
	}
	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		!strcmp(status, "disabled")) {
		return 0;
	}
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &(dai_dev.i2c_dev))) {
		return 0;
	}
	fdt_get_property_u_32_index(np, "slave-addr", 0, &i2c_slave_addr);
	snd_soc_register_dai(&cl3016_for_i2si_dai);
	//printf("\n************cl3016_dai_init end ******************\n");
	return 0;
}

int reset_cl3016(void)
{
	//open clock;
	struct snd_pcm_params i2si_params = {0};
	int fd,i2si_fd;
	int res = 0;
	//printf("\n************reset_cl3016 start ******************\n");

	if (!dai_dev.i2c_dev) {
		return 0;
	}

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(i2si_fd < 0){
		printf("Open /dev/sndC0i2si fail\n");
		return -1;
	}
	i2si_params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	i2si_params.format = SND_PCM_FORMAT_S24_LE;
	i2si_params.sync_mode = 0;
	i2si_params.align = 0;
	i2si_params.rate = 48000;
	i2si_params.channels = 2;
	i2si_params.period_size = 1536;
	i2si_params.periods = 8;
	i2si_params.bitdepth = 32;
	i2si_params.start_threshold = 1;
	i2si_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	i2si_params.pcm_dest = SND_PCM_DEST_BYPASS;
	res = ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &i2si_params);
	if (res < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}
	ioctl(i2si_fd, SND_IOCTL_START, 0);
	ioctl(i2si_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
	close(i2si_fd);

	
	gpio_configure(PINPAD_R01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_R01, 0);
	msleep(200);
	gpio_set_output(PINPAD_R01, 1);
	msleep(200);
	

	fd = open(dai_dev.i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc3016 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	cl3016_Write_Reg(fd, 0x62, 0x95);
	cl3016_Write_Reg(fd, 0x00, 0x68);
	cl3016_Write_Reg(fd, 0x01, 0x58);
	cl3016_Write_Reg(fd, 0x05, 0x86);
	cl3016_Write_Reg(fd, 0x02, 0x13);
	cl3016_Write_Reg(fd, 0x21, 0x00);
	cl3016_Write_Reg(fd, 0x22, 0x00);
	cl3016_Write_Reg(fd, 0x03, 0x20);
	cl3016_Write_Reg(fd, 0x06, 0x04);
	cl3016_Write_Reg(fd, 0x0a, 0x44);//for linein
	cl3016_Write_Reg(fd, 0x39, 0x14);
	cl3016_Write_Reg(fd, 0x1b, 0x03);
	cl3016_Write_Reg(fd, 0x1e, 0x31);
	cl3016_Write_Reg(fd, 0x30, 0x20);
	cl3016_Write_Reg(fd, 0x2c, 0xb1);
	cl3016_Write_Reg(fd, 0x20, 0x05);
	cl3016_Write_Reg(fd, 0x46, 0x90);
	cl3016_Write_Reg(fd, 0x41, 0x01);
	cl3016_Write_Reg(fd, 0x33, 0x78);
	cl3016_Write_Reg(fd, 0x0b, 0x00);
	cl3016_Write_Reg(fd, 0x0d, 0x00);

#if 0
	cl3016_i2c_config_is_right(fd, 0x62, 0x95);
	cl3016_i2c_config_is_right(fd, 0x00, 0x68);
	cl3016_i2c_config_is_right(fd, 0x01, 0x58);
	cl3016_i2c_config_is_right(fd, 0x05, 0x86);
	cl3016_i2c_config_is_right(fd, 0x02, 0x13);
	cl3016_i2c_config_is_right(fd, 0x21, 0x0c);
	cl3016_i2c_config_is_right(fd, 0x22, 0x0c);
	cl3016_i2c_config_is_right(fd, 0x03, 0x20);
	cl3016_i2c_config_is_right(fd, 0x06, 0x04);
	cl3016_i2c_config_is_right(fd, 0x0a, 0x44);
	cl3016_i2c_config_is_right(fd, 0x39, 0x14);
	cl3016_i2c_config_is_right(fd, 0x1b, 0x03);
	cl3016_i2c_config_is_right(fd, 0x1e, 0x31);
	cl3016_i2c_config_is_right(fd, 0x30, 0x20);
	cl3016_i2c_config_is_right(fd, 0x2c, 0xb1);
	cl3016_i2c_config_is_right(fd, 0x20, 0x05);
	cl3016_i2c_config_is_right(fd, 0x46, 0x90);
	cl3016_i2c_config_is_right(fd, 0x41, 0x01);
	cl3016_i2c_config_is_right(fd, 0x33, 0x78);
	cl3016_i2c_config_is_right(fd, 0x0b, 0x00);
	cl3016_i2c_config_is_right(fd, 0x0d, 0x00);
#endif
	close (fd);
	//printf("\n************reset_cl3016 end ******************\n");
	return 0;
}
__initcall(reset_cl3016);

#if 0
static int reg_set(int argc, char *argv[])
{
	int fd, res;
	uint8_t addr, val;

	gpio_configure(PINPAD_R01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_R01, 0);
	msleep(200);
	gpio_set_output(PINPAD_R01, 1);
	msleep(200);

	fd = open("/dev/gpio-i2c3",O_RDWR);
	if (fd < 0) {
		printf("gpio-i2c3 open error\n");
		return -1;
	}
	addr = strtoul(argv[1], NULL, 16);
	val = strtoul(argv[2], NULL, 16);
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);
	printf("3016 i2c3 fd %d, set reg 0x%x to 0x%x\n",fd, addr, val);
	res = cl3016_Write_Reg(fd, addr, val);
	printf("reg res %d\n", res);
	cl3016_i2c_config_is_right(fd, addr, val);
	close(fd);
	return 0;
}

CONSOLE_CMD(reg, NULL, reg_set, CONSOLE_CMD_MODE_SELF,
	"reg addr val")
#endif
