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

struct dai_device {
	struct snd_capability cap;
	const char *i2c_dev;
};

static u32 i2c_slave_addr = 0;
/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K                                                    \
	(SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | SND_PCM_RATE_48000 |        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000)

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

#define SND_IN 1
#define SND_OUT 2
static int snd_direction = 0;

static int es_write_reg(int fd, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
{
	int ret = 0;
	uint8_t wdata[2] = {0,0};
	wdata[0] = reg_addr;
	wdata[1] = data;
	struct i2c_transfer_s xfer;
	struct i2c_msg_s i2c_msg = {0};
	xfer.msgv = &i2c_msg;
	xfer.msgv->addr = slave_addr;
	xfer.msgv->flags = 0x0;
	xfer.msgv->buffer = wdata;
	xfer.msgv->length = 2;
	xfer.msgc         = 1;

	ret = ioctl(fd,I2CIOC_TRANSFER,&xfer);
	if (ret < 0) {
		printf ("i2c write failed\n");
	}
	return ret;
}

static int cjc8990_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = 0x1B;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_write_reg(fd, slave_addr, reg_addr, pdata);
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

static int cjc8990_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = 0x1B;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int cjc8990_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	int ret;
	/* cjc8990 spec */
	if (chip_addr % 2 != 0) {
		chip_addr -= 1;
	}

	read_data[0] = chip_addr;
	ret = cjc8990_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val || ret < 0) {
		printf("cjc8990_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return ret;
}

static int do_not_cfg = 0;
static int cjc8990_dai_hw_params(struct snd_soc_dai *dai, struct snd_pcm_params *params)
{
	int res = 0;
	int fd = 0;
	uint8_t val = 0;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;
	return 0;
	if (do_not_cfg)
		return 0;
	snd_pcm_format_t fmt = params->format;

	log_i("cjc8990_dai_hw_params, %d, %d, %d\n", (int)rate, (int)fmt, (int)channels);
	fd = open(dai_dev->i2c_dev,O_RDWR);
	if(fd < 0){
		printf("cjc8990 open %s error\n", dai_dev->i2c_dev);
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	log_i("dai->name %s\n", dai->name);
	if (!strcmp("cjc8990-for-i2so-dai", dai->name)) {
		log_i("cfg snd out\n");
		snd_direction |= SND_OUT;
	} else if (!strcmp("cjc8990-for-i2si-dai", dai->name)) {
		log_i("cfg snd in\n");
		snd_direction |= SND_IN;
	}

	/* R7, bit7, 0 = BCLK not inverted; bit 6, 0 = Enable Slave Mode; 
	 * bit5, 1 = swap left and right DAC data; bit4, LRCLK polarity or pcm A/B select;
	 * bit3-2 Word Length 11: 32bit 10: 24bit 01: 20bit  00:16bit
	 * bit1-0 Format Select 11: pcm mode 10: i2s mode 01: Left justified
	 */
	val = 2;//i2s mode
	if (fmt == SND_PCM_FORMAT_S16_LE) {
		val |= (0 << 2);
	} else if (fmt == SND_PCM_FORMAT_S24_LE) {
		val |= (2 << 2);
	} else if (fmt == SND_PCM_FORMAT_S32_LE) {
		val |= (3 << 2);
	}
	res += cjc8990_Write_Reg(fd, 0x0E, val);

	if (snd_direction & SND_IN) {
		res += cjc8990_Write_Reg(fd, 0x01, 0x1f);//bit 7: 1 mute; bits 0-5: vol
		res += cjc8990_Write_Reg(fd, 0x2B, 0xC3);//bits 0-7: vol
	}

	if (snd_direction & SND_OUT) {
		res +=  cjc8990_Write_Reg(fd, 0x05, 0x79);//bits 0-6: vol
		res +=  cjc8990_Write_Reg(fd, 0x07, 0x79);//bits 0-6: vol
		res +=  cjc8990_Write_Reg(fd, 0x15, 0xff);//bits 0-7: vol
		res +=  cjc8990_Write_Reg(fd, 0x17, 0xff);//bits 0-7: vol
	}

	if (res < 0) {
		log_e("cjc8990 i2c config error\n");
		close(fd);
		return 0;
	}

	close(fd);
	log_i("cjc8990_dai_hw_params done, snd_direction %d\n", snd_direction);
	return 0;
}

static int cjc8990_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cjc8990_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int cjc8990_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cjc8990_dai_hw_free(struct snd_soc_dai *dai)
{
	int fd;
	struct dai_device *dai_dev = (struct dai_device *)dai->priv;

	return 0;
	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8990 open %s error\n", dai_dev->i2c_dev);
		return -1;
	}
	ioctl(fd, I2CIOC_TIMEOUT, 100);

	log_i("free dai->name %s\n", dai->name);
	if (!strcmp("cjc8990-for-i2so-dai", dai->name)) {
		snd_direction &= ~SND_OUT;
		/* mute lineout & dac */
		cjc8990_Write_Reg(fd, 0x05, 0x00);//bits 0-6: vol
		cjc8990_Write_Reg(fd, 0x07, 0x00);//bits 0-6: vol
		/* R10, bit 7:0, 0000 0000 = Digital Mute(-127.5), ... 0.5dB steps up to 1111 1111 = 0dB*/
		cjc8990_Write_Reg(fd, 0x15, 0x00);//bits 0-7: vol
		cjc8990_Write_Reg(fd, 0x17, 0x00);//bits 0-7: vol
	} else if (!strcmp("cjc8990-for-i2si-dai", dai->name)) {
		snd_direction &= ~SND_IN;
		/* mute linein & adc */
		cjc8990_Write_Reg(fd, 0x01, 0x80);//bit 7: 1 mute; bits 0-5: vol
		cjc8990_Write_Reg(fd, 0x2B, 0x00);//bits 0-7: vol
	}

	close (fd);
	return 0;

}

static struct snd_soc_dai_driver cjc8990_dai_driver = {
	.hw_params = cjc8990_dai_hw_params,
	.trigger = cjc8990_dai_trigger,
	.ioctl = cjc8990_dai_ioctl,
	.get_capability = cjc8990_dai_get_capability,
	.hw_free = cjc8990_dai_hw_free,
};

static struct snd_soc_dai cjc8990_for_i2so_dai = {
	.name = "cjc8990-for-i2so-dai",
	.driver = &cjc8990_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai cjc8990_for_i2si_dai = {
	.name = "cjc8990-for-i2si-dai",
	.driver = &cjc8990_dai_driver,
	.priv = &dai_dev,
};

int cjc8990_dai_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/cjc8990");
	if (np < 0) {
		return 0;
	}

	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &dai_dev.i2c_dev)) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		!strcmp(status, "disabled")) {
		return 0;
	}

	fdt_get_property_u_32_index(np, "slave-addr", 0, &i2c_slave_addr);

	snd_soc_register_dai(&cjc8990_for_i2so_dai);
	snd_soc_register_dai(&cjc8990_for_i2si_dai);
	return 0;
}

#include <kernel/module.h>
int reset_cjc8990(void)
{
	//open clock;
	struct snd_pcm_params params = {0};
	int fd;

	if (!dai_dev.i2c_dev) {
		return 0;
	}

	fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0)
		return 0;

	do_not_cfg = 1;

	params.channels = 1;
	params.period_size = 1536;
	params.align = 0;
	params.rate = 48000;
	params.periods = 8;
	params.bitdepth = 16;
	params.start_threshold = 2;
	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = 0;
	snd_pcm_uframes_t poll_size = 1536;

	ioctl(fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(fd, SND_IOCTL_AVAIL_MIN, &poll_size);
	ioctl(fd, SND_IOCTL_START, 0);
	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);
	close(fd);

	do_not_cfg = 0;

	//open cjc8988's dac & adc
	fd = open(dai_dev.i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8990 i2c open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	/* R5, ADC & DAC control */
	cjc8990_Write_Reg(fd, 0x0A, 0x08);

	//mute all
	cjc8990_Write_Reg(fd, 0x01, 0x80);//LI, anolog mute
	cjc8990_Write_Reg(fd, 0x2B, 0x00);//LI, digital mute
	cjc8990_Write_Reg(fd, 0x05, 0x00);//LO, mute
	cjc8990_Write_Reg(fd, 0x07, 0x00);//RO, mute
	cjc8990_Write_Reg(fd, 0x15, 0x00);
	cjc8990_Write_Reg(fd, 0x17, 0x00);
	printf("only power on cjc8990 vmid\n");
	cjc8990_Write_Reg(fd, 0x33, 0x00);
	msleep(200);

	cjc8990_Write_Reg(fd, 0x1E, 0x00);
	cjc8990_Write_Reg(fd, 0x0A, 0x00);
	cjc8990_Write_Reg(fd, 0x10, 0x00);
	cjc8990_Write_Reg(fd, 0x18, 0x0F);
	cjc8990_Write_Reg(fd, 0x1A, 0x0F);
	cjc8990_Write_Reg(fd, 0x20, 0x00);
	cjc8990_Write_Reg(fd, 0x22, 0x7B);
	cjc8990_Write_Reg(fd, 0x24, 0x00);
	cjc8990_Write_Reg(fd, 0x26, 0x32);
	cjc8990_Write_Reg(fd, 0x28, 0x00);
	/* R23, DAC mono mix, bit5:4, 00: stereo, 01: DACL, 01: DACR 11: mono ((L+R)/2) into DACL and DACR*/
	cjc8990_Write_Reg(fd, 0x2E, 0x00);//stereo
	cjc8990_Write_Reg(fd, 0x30, 0x04);
	cjc8990_Write_Reg(fd, 0x36, 0x00);
	cjc8990_Write_Reg(fd, 0x40, 0x00);
	cjc8990_Write_Reg(fd, 0x42, 0x0A);
	cjc8990_Write_Reg(fd, 0x44, 0x0A);
	cjc8990_Write_Reg(fd, 0x47, 0x00);
	cjc8990_Write_Reg(fd, 0x48, 0x80);
	cjc8990_Write_Reg(fd, 0x4A, 0x00);
	cjc8990_Write_Reg(fd, 0x86, 0x00);

	cjc8990_Write_Reg(fd, 0x01, 0x1f);//bit 7: 1 mute; bits 0-5: vol
	cjc8990_Write_Reg(fd, 0x2B, 0xC3);//bits 0-7: vol
	cjc8990_Write_Reg(fd, 0x05, 0x79);//bits 0-6: vol
	cjc8990_Write_Reg(fd, 0x07, 0x79);//bits 0-6: vol
	cjc8990_Write_Reg(fd, 0x15, 0xff);//bits 0-7: vol
	cjc8990_Write_Reg(fd, 0x17, 0xff);//bits 0-7: vol

	printf("power on cjc8990 adc && dac\n");
	/* R26 Pwr Mgmt (2), ADC*/
	cjc8990_Write_Reg(fd, 0x33, 0x68);
	cjc8990_i2c_config_is_right(fd, 0x33, 0x68);
	/* R26 Pwr Mgmt (2), DAC*/
	cjc8990_Write_Reg(fd, 0x35, 0xE0);
	cjc8990_i2c_config_is_right(fd, 0x35, 0xE0);

	/* R5, ADC & DAC control */
	msleep(200);
	cjc8990_Write_Reg(fd, 0x0A, 0x00);

	return 0;
}

__initcall(reset_cjc8990);


#if 0
static int spo_fd = -1;
int spdif_open(int rate, int channels, int bitdepth, int format)
{
	struct snd_pcm_params params = {0};

	if (spo_fd >= 0) {
		return -1;
	}

	spo_fd = open("/dev/sndC0spo", O_WRONLY);
	if (spo_fd < 0) {
		printf("open spo failed\n");
		return spo_fd;
	}

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.rate = rate;//i2si 多少就填多少
	params.channels = channels;//i2si 多少就填多少
	params.bitdepth = bitdepth;//i2si 多少就填多少
	params.format = format;//i2si 多少就填多少
	params.sync_mode = 0;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.period_size = 1536;
	params.periods = 16;
	params.start_threshold = 1;
	params.pcm_source = SND_SPO_SOURCE_I2SODMA;
	ioctl(spo_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(spo_fd, SND_IOCTL_START, 0);
	printf("spo start\n");
	return spo_fd;
}

/* audio path: cvbs audio--> ADC --> i2si --> i2so -->DAC --> speaker
 *	i2si : L01;
 *   start i2si
 */
int i2si_fd = -1, i2so_fd = -1;
static int cvbs_audio_open(int argc, char *argv[])
{
	struct snd_pcm_params i2si_params = {0};
	int channels = 1;

	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int read_size = period_size;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;
	struct snd_pcm_params params = {0};

	if((i2si_fd >= 0) && (i2so_fd >= 0))
		return 0;

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(i2si_fd < 0){
		printf("Open /dev/sndC0i2si fail\n");
		return -1;
	}

	printf("ioctl````\n");
	i2si_params.access = access;
	i2si_params.format = format;
	i2si_params.sync_mode = 0;
	i2si_params.align = align;
	i2si_params.rate = rate;
	i2si_params.channels = channels;
	i2si_params.period_size = period_size;
	i2si_params.periods = periods;
	i2si_params.bitdepth = bitdepth;
	i2si_params.start_threshold = 1;
	i2si_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	i2si_params.pcm_dest = SND_PCM_DEST_BYPASS;
	ret = ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &i2si_params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (i2so_fd < 0) {
		printf("Open /dev/sndC0i2so error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = 0;
	params.align = align;
	params.rate = rate;

	params.channels = channels;
	params.period_size = read_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 2;
	ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(i2so_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(i2si_fd, SND_IOCTL_START, 0);
	printf ("i2si_fd SND_IOCTL_START done\n");

	ioctl(i2so_fd, SND_IOCTL_START, 0);
	printf ("i2so_fd SND_IOCTL_START done\n");

	spdif_open(rate, 2, bitdepth, SND_PCM_FORMAT_S16_LE);

	(void)argc;
	(void)argv;
	return 0;
}

static int cvbs_audio_close(int argc, char *argv[])
{
	printf(">>> cvbs_audio_close\n");
	if(i2so_fd >= 0)
	{
		printf ("cvbs_audio_close --i2so_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2so_fd, SND_IOCTL_DROP, 0);
		ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0);
		close(i2so_fd);
		i2so_fd = -1;
	}

	if(i2si_fd >= 0){
		printf ("cvbs_audio_close --i2si_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2si_fd, SND_IOCTL_DROP, 0);
		ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
		close(i2si_fd);
		i2si_fd = -1;
	}

	if(spo_fd >= 0){
		printf ("cvbs_audio_close --i2si_fd, SND_IOCTL_HW_FREE \n");
		ioctl(spo_fd, SND_IOCTL_DROP, 0);
		ioctl(spo_fd, SND_IOCTL_HW_FREE, 0);
		close(spo_fd);
		spo_fd = -1;
	}


	(void)argc;
	(void)argv;
	return 0;
}

CONSOLE_CMD(cvbs_on, NULL, cvbs_audio_open, CONSOLE_CMD_MODE_SELF,
	"cvbs_on")

CONSOLE_CMD(cvbs_off, NULL, cvbs_audio_close, CONSOLE_CMD_MODE_SELF,
	"cvbs_off")

static int reg_set(int argc, char *argv[])
{
	int fd, res;
	uint8_t addr, val;

	(void)argc;
	(void)argv;
	if (argc < 3) {
		return 0;
	}

	if (!dai_dev.i2c_dev) {
		printf("no i2c dev\n");
		return -1;
	}

	fd = open(dai_dev.i2c_dev,O_RDWR);
	if (fd < 0) {
		printf("i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	addr = strtoul(argv[1], NULL, 16);
	val = strtoul(argv[2], NULL, 16);
	printf("i2c1 fd %d, set reg 0x%x to 0x%x\n",fd, addr, val);
	res = cjc8990_Write_Reg(fd, addr, val);
	printf("reg res %d\n", res);
	cjc8990_i2c_config_is_right(fd, addr, val);

	close(fd);
	return 0;
}

CONSOLE_CMD(reg, NULL, reg_set, CONSOLE_CMD_MODE_SELF,
	"reg addr val")

#include <hcuapi/gpio.h>
static int gpio_set(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	if (argc < 3) {
		return 0;
	}
	printf("set gpio %d to %d\n", atoi(argv[1]), atoi(argv[2]));
	gpio_configure(atoi(argv[1]), GPIO_DIR_OUTPUT);
	gpio_set_output(atoi(argv[1]), atoi(argv[2]));

	return 0;
}

CONSOLE_CMD(gpio, NULL, gpio_set, CONSOLE_CMD_MODE_SELF,
	"gpio pin val")
#endif

