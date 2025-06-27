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
#include <kernel/lib/fdt_api.h>

#define I2C_7BIT_ADDR 0x13

struct dai_device {
	struct snd_capability cap;
	const char *i2c_dev;
	int fd;
};

static u32 i2c_slave_addr = 0;

#define SND_PCM_RATE_32K48K                                                    \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 |        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000 |       \
	 SND_PCM_RATE_192000)

#define SND_PCM_RATE_44_1K                                                     \
	(SND_PCM_RATE_5512 | SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | \
	SND_PCM_RATE_44100 | SND_PCM_RATE_88200 | SND_PCM_RATE_176400)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats =
				(SND_PCM_FMTBIT_S8 | SND_PCM_FMTBIT_U8 |
				 SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_U16_LE |
				 SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_U24_LE),
		},
};

static int es_write_reg(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
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

static int es_read_reg(int fd, uint8_t slave_addr, uint8_t *data)
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

static int es7243i_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = I2C_7BIT_ADDR;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_write_reg(fd, slave_addr, reg_addr, pdata);
}

static int es7243i_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = I2C_7BIT_ADDR;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int es7243i_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	read_data[0] = chip_addr;
	es7243i_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val) {
		printf("es7243i_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return 0;
}

static int es7243i_dai_hw_params(struct snd_soc_dai *dai, struct snd_pcm_params *params)
{
	int res = 0;
	int fd = 0;
	struct dai_device *dev = dai->priv;

	fd = open(dai_dev.i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("es7243i:i2c open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);
	dev->fd = fd;
	res = es7243i_Write_Reg(fd, 0x00, 0x01);
	if (res < 0) {
		printf("es7243i i2c config error\n");
		return -1;
	}
	vTaskDelay(1);
	
	res +=	es7243i_Write_Reg(fd, 0x06, 0x00);

	res +=  es7243i_Write_Reg(fd, 0x05, 0x1B);

	res +=  es7243i_Write_Reg(fd, 0x01, 0x8F);  //0x8F: DSP mode 16bit; 0x0C: I2S mode 16bit
	es7243i_i2c_config_is_right(fd, 0x01, 0x8F);
	
	res +=  es7243i_Write_Reg(fd, 0x08, 0x11);
	es7243i_i2c_config_is_right(fd, 0x08, 0x11);//gain set to 1dB,input <= 5.0 Vpp

	res +=  es7243i_Write_Reg(fd, 0x05, 0x13);
	if (res < 0) {
		printf("es7243i i2c config error\n");
		return -1;
	}

	vTaskDelay(1000);
	return 0;
}

static int es7243i_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int es7243i_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int es7243i_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int es7243i_dai_hw_free(struct snd_soc_dai *dai)
{
	struct dai_device *dev = dai->priv;

	close(dev->fd);

	return 0;
}

static struct snd_soc_dai_driver es7243i_dai_driver = {
	.hw_params = es7243i_dai_hw_params,
	.trigger = es7243i_dai_trigger,
	.ioctl = es7243i_dai_ioctl,
	.get_capability = es7243i_dai_get_capability,
	.hw_free = es7243i_dai_hw_free,
};

static struct snd_soc_dai es7243i_for_i2si_dai = {
	.name = "es7243i-for-tdmi-dai",
	.driver = &es7243i_dai_driver,
	.priv = &dai_dev,
};

int es7243i_dai_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/es7243");
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

	return snd_soc_register_dai(&es7243i_for_i2si_dai);
}

