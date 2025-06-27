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

struct dai_device {
	struct snd_capability cap;
	const char *i2c_dev;
	const char *input1_alias;
	const char *input2_alias;
	int input;
};

static u32 i2c_slave_addr = 0;

/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K (SND_PCM_RATE_48000)

#define SND_PCM_RATE_44_1K (SND_PCM_RATE_44100)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats = (SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_S32_LE),
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

static int cjc8988_Write_Reg(int fd, uint8_t reg_addr, uint8_t pdata)
{
	uint8_t slave_addr = 0x1A;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_write_reg(fd, slave_addr, reg_addr, pdata);
}

static int cjc8988_read_Reg(int fd, uint8_t *pdata)
{
	uint8_t slave_addr = 0x1A;
	if(i2c_slave_addr != 0){
		slave_addr = (uint8_t)i2c_slave_addr;
	}
	return es_read_reg(fd, slave_addr, pdata);
}

static uint8_t read_data[2] = {0};

static int cjc8988_i2c_config_is_right(int fd, uint8_t chip_addr, uint8_t val)
{
	int ret;
	/* cjc8998 spec */
	if (chip_addr % 2 != 0) {
		chip_addr -= 1;
	}

	read_data[0] = chip_addr;
	ret = cjc8988_read_Reg(fd, &read_data[0]);

	if (read_data[1] != val || ret < 0) {
		printf("cjc8988_Write_Reg 0x%x error\n", chip_addr);
	} else {
		printf("chip_addr 0x%x: val %d is right\n", chip_addr, val);
	}

	return ret;
}

#if 0
static struct work_s cjc8988_work = { 0 };
static void cjc8988_dai_delayed_trig(void *parameter)
{
	int fd;
	struct dai_device *dai_dev = (struct dai_device *)parameter;
	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		return;
	}
	//printf("set adc analogue volume\n");
	/* R25 Pwr Mgmt (1), ADC*/
	//printf("power on adc && vmid\n");
	cjc8988_Write_Reg(fd, 0x01, 0x10);//L, 0dB
	cjc8988_Write_Reg(fd, 0x03, 0x10);//R, 0dB
	//cjc8988_Write_Reg(fd, 0x33, 0x7C);
	close(fd);
}
#endif

static int do_not_cfg = 0;
static int cjc8988_dai_hw_params(struct snd_soc_dai *dai, struct snd_pcm_params *params)
{
	int res = 0;
	int fd = 0;
	int8_t val;
	struct dai_device *dai_dev = dai->priv;
	snd_pcm_format_t fmt = params->format;

	if (do_not_cfg)
		return 0;

	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8988 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	printf("dai->name %s\n", dai->name);
	if (!strcmp("cjc8988-for-i2so-dai", dai->name)) {
		snd_direction |= SND_OUT;
	} else if (!strcmp("cjc8988-for-i2si-dai", dai->name)) {
		snd_direction |= SND_IN;
	}

	switch(params->rate) {
		case 48000: //384
			cjc8988_Write_Reg(fd, 0x10, 0x02);
			break;
		case 44100: //384
			cjc8988_Write_Reg(fd, 0x10, 0x22);
			break;
		default:
			printf("unsupport samplerate\n");
			cjc8988_Write_Reg(fd, 0x10, 0x00);
			return -EINVAL;
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
	res += cjc8988_Write_Reg(fd, 0x0E, val);

	/* R32 ADC SignalPath Control (Left), R33 ADC SignalPath Control (Right))
	 * bit7:6, 00 = LINPUT1, 01 = LINPUT2; bit5:4, Left Channel Microphone Gain Boost, 00 = Boost off
	 */
	if (dai_dev->input == 0) {
		res += cjc8988_Write_Reg(fd, 0x40, 0x00);
		res += cjc8988_Write_Reg(fd, 0x42, 0x00);
	} else {
		res += cjc8988_Write_Reg(fd, 0x40, 0x40);
		res += cjc8988_Write_Reg(fd, 0x42, 0x40);
	}

	if (snd_direction & SND_IN) {
		printf("set adc analogue volume\n");
		/* R0 & R1, bit7: 1, mute; bit0-5: vol, 000000 = -17.25dB,
		 * 0.75dB steps up to 111111 = +30dB */
		res += cjc8988_Write_Reg(fd, 0x01, 0x17);//L, 0dB
		res += cjc8988_Write_Reg(fd, 0x03, 0x17);//R, 0dB
		/* R21, bit 7:0, Left ADC volume 0000 0000 = Digital Mute(-97.5dB), 0.5dB steps up to 1111 1111 = +30dB */
		/* R22, bit 7:0, Right ADC volume 0000 0000 = Digital Mute(-97.5dB), 0.5dB steps up to 1111 1111 = +30dB */
		res += cjc8988_Write_Reg(fd, 0x2B, 0xc3);//L, 0dB
		res += cjc8988_Write_Reg(fd, 0x2D, 0xc3);//R, 0dB
	}

	if (snd_direction & SND_OUT) {
		printf("set dac analogue volume\n");
		/* R2 & R3, bit0-6: Analogue vol, 1111111 = +6dB, 80 steps to 0110000 = -67dB,
		 * 0110000 to 0000000 = Analogue MUTE*/
		res += cjc8988_Write_Reg(fd, 0x05, 0x79);//L, 0dB
		res += cjc8988_Write_Reg(fd, 0x07, 0x79);//R, 0dB
	}

	if (res < 0) {
		printf("cjc8988 i2c config error\n");
		close (fd);
		return 0;
	}

	printf("cjc8988 i2c config done!\n");
	close (fd);

	return 0;
}

static int cjc8988_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cjc8988_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	struct dai_device *dai_dev = dai->priv;

	switch (cmd) {
	case SND_IOCTL_SET_CJC8988_INPUT:
		if(arg) {
			printf("set 8988 src mode\n");
			struct snd_cjc8988_input *src = (struct snd_cjc8988_input *)arg;
			if(!strcmp(src->input_name, dai_dev->input1_alias)) {
				dai_dev->input = 0;
				printf("set input1\n");
			} else if(!strcmp(src->input_name, dai_dev->input2_alias)) {
				dai_dev->input = 1;
				printf("set input2\n");
			} 
		}
		break;
	default:
		break;
	}

	return 0;
}

static int cjc8988_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cjc8988_dai_hw_free(struct snd_soc_dai *dai)
{
	int fd;
	struct dai_device *dai_dev = dai->priv;

	fd = open(dai_dev->i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8988 i2c1 open error\n");
		return -1;
	}
	ioctl(fd,I2CIOC_TIMEOUT,100);

	printf("free dai->name %s\n", dai->name);
	/* only mute line in, do not mute line out */
	if (!strcmp("cjc8988-for-i2so-dai", dai->name)) {
		snd_direction &= ~SND_OUT;
		//cjc8988_Write_Reg(fd, 0x05, 0x00);//LO, mute
		//cjc8988_Write_Reg(fd, 0x07, 0x00);//RO, mute
		//printf("mute dac\n");
	} else if (!strcmp("cjc8988-for-i2si-dai", dai->name)) {
		snd_direction &= ~SND_IN;
		//cjc8988_Write_Reg(fd, 0x01, 0x80);//LI, anolog mute
		//cjc8988_Write_Reg(fd, 0x03, 0x80);//RI, anolog mute
		cjc8988_Write_Reg(fd, 0x2B, 0x00);//LI, digital mute
		cjc8988_Write_Reg(fd, 0x2D, 0x00);//RI, digital mute
		printf("mute adc\n");
	}

	close (fd);
	return 0;
}

static struct snd_soc_dai_driver cjc8988_dai_driver = {
	.hw_params = cjc8988_dai_hw_params,
	.trigger = cjc8988_dai_trigger,
	.ioctl = cjc8988_dai_ioctl,
	.get_capability = cjc8988_dai_get_capability,
	.hw_free = cjc8988_dai_hw_free,
};

static struct snd_soc_dai cjc8988_for_i2so_dai = {
	.name = "cjc8988-for-i2so-dai",
	.driver = &cjc8988_dai_driver,
	.priv = &dai_dev,
};

static struct snd_soc_dai cjc8988_for_i2si_dai = {
	.name = "cjc8988-for-i2si-dai",
	.driver = &cjc8988_dai_driver,
	.priv = &dai_dev,
};

int cjc8988_dai_init(void)
{
	int np;
	const char *status;
	const char *input_default;
	dai_dev.input = 0;
	dai_dev.input1_alias = NULL;
	dai_dev.input2_alias = NULL;
	dai_dev.i2c_dev = NULL;

	np = fdt_node_probe_by_path("/hcrtos/cjc8988");
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

	fdt_get_property_string_index(np, "input1", 0, &(dai_dev.input1_alias));
	fdt_get_property_string_index(np, "input2", 0, &(dai_dev.input2_alias));
	fdt_get_property_string_index(np, "input-default", 0, &input_default);
	if ((input_default && !strcmp("input1", input_default)) ||
		(input_default && dai_dev.input1_alias && !strcmp(dai_dev.input1_alias, input_default))) {
		dai_dev.input = 0;
	} else if ((input_default && !strcmp("input2", input_default)) ||
		(input_default && dai_dev.input2_alias && !strcmp(dai_dev.input2_alias, input_default))) {
		dai_dev.input = 1;
	} 

	snd_soc_register_dai(&cjc8988_for_i2so_dai);
	snd_soc_register_dai(&cjc8988_for_i2si_dai);
	return 0;
}

#include <kernel/module.h>
int reset_cjc8988(void)
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

	params.channels = 2;
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
		printf("cjc8988 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	/* R5, ADC & DAC control */
	cjc8988_Write_Reg(fd, 0x0A, 0x08);

	//mute all
	cjc8988_Write_Reg(fd, 0x01, 0x80);//LI, anolog mute
	cjc8988_Write_Reg(fd, 0x03, 0x80);//RI, anolog mute
	cjc8988_Write_Reg(fd, 0x2B, 0x00);//LI, digital mute
	cjc8988_Write_Reg(fd, 0x2D, 0x00);//RI, digital mute
	cjc8988_Write_Reg(fd, 0x05, 0x00);//LO, mute
	cjc8988_Write_Reg(fd, 0x07, 0x00);//RO, mute
	printf("only power on vmid\n");
	cjc8988_Write_Reg(fd, 0x33, 0x00);
	msleep(200);

	/* R8, only used when cjc8988 works on master mode */
	cjc8988_Write_Reg(fd, 0x10, 0x00);

	/* R10, bit 7:0, 0000 0000 = Digital Mute(-127.5), ... 0.5dB steps up to 1111 1111 = 0dB*/
	cjc8988_Write_Reg(fd, 0x15, 0xFF);
	/* R11, bit 7:0, 0000 0000 = Digital Mute(-127.5), ... 0.5dB steps up to 1111 1111 = 0dB*/
	cjc8988_Write_Reg(fd, 0x17, 0xFF);

	/* R12, BASS */
	cjc8988_Write_Reg(fd, 0x18, 0x0F);
	/* R13, bit6: Treble Filter, 0 = High Cutoff (8kHz at 48kHz sampling), 1 = Low Cutoff (4kHz at 48kHz sampling);
	 * bit0-3: Treble Intensity, 0000 or 0001 = +9dB, 1.5dB steps, 1111 = Disable */
	cjc8988_Write_Reg(fd, 0x1A, 0x0F);//DAC power select

	/* R16 */
	cjc8988_Write_Reg(fd, 0x20, 0x00);

	/* R20 */
	cjc8988_Write_Reg(fd, 0x28, 0x00);

	/* R23, DAC mono mix, bit5:4, 00: stereo, 01: DACL, 01: DACR 11: mono ((L+R)/2) into DACL and DACR*/
	cjc8988_Write_Reg(fd, 0x2E, 0x00);//stereo

	/* R24 */
	cjc8988_Write_Reg(fd, 0x31, 0x80);//bit 7: Enable Common Mode Feedback

	/* R27 VREF to analogue output resistance */
	cjc8988_Write_Reg(fd, 0x36, 0x00);

	/* R31 ADC input Mode, bit7:6, 00: Stereo, 01: Analogue Mono Mix (using left ADC), 10: Analogue Mono Mix (using right ADC), 11: Digital Mono Mix*/
	cjc8988_Write_Reg(fd, 0x3E, 0x00);

	/* R34 Left ADC to Left Mixer, bit2:0, 000 = LINPUT1, 001 = LINPUT2, 010 = Reserved, 011 = Left ADC Input (after PGA / MICBOOST) */
	/* bit6:4, LI2LOVOL */
	cjc8988_Write_Reg(fd, 0x45, 0x73);//Left ADC link to PGA

	/* R35 Right DAC to Left Mixer*/
	cjc8988_Write_Reg(fd, 0x46, 0x70);

	/* R36 Left ADC to Right Mixer,  bit2:0, 000 = LINPUT1, 001 = LINPUT2, 010 = Reserved, 011 = Right ADC Input (after PGA / MICBOOST) */
	cjc8988_Write_Reg(fd, 0x48, 0x73);//Right ADC link to PGA

	/* R37 Right DAC to Right Mixer*/
	cjc8988_Write_Reg(fd, 0x4B, 0x70);

	/* R40 LOUT2VOL*/
	cjc8988_Write_Reg(fd, 0x51, 0x79);
	/* R41 OUT2VOL*/
	cjc8988_Write_Reg(fd, 0x53, 0x79);

	/* R67 DACMIX BIAS*/
	cjc8988_Write_Reg(fd, 0x86, 0x08);//bit 3, low bias current (results in lower performance and power consumption)

	printf("power on adc && dac\n");
	/* R26 Pwr Mgmt (2), ADC*/
	cjc8988_Write_Reg(fd, 0x33, 0x7C);
	cjc8988_i2c_config_is_right(fd, 0x33, 0x7C);
	/* R26 Pwr Mgmt (2), DAC*/
	cjc8988_Write_Reg(fd, 0x35, 0xF8);
	cjc8988_i2c_config_is_right(fd, 0x35, 0xF8);

	/* R5, ADC & DAC control */
	msleep(200);
	cjc8988_Write_Reg(fd, 0x0A, 0x00);
	close (fd);
	return 0;
}

__initcall(reset_cjc8988);

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

#if 0
static int reg_set(int argc, char *argv[])
{
	int fd, res;
	uint8_t addr, val;

	(void)argc;
	(void)argv;
	if (argc < 3) {
		return 0;
	}

	fd = open(dai_dev.i2c_dev, O_RDWR);
	if (fd < 0) {
		printf("cjc8988 i2c1 open error\n");
		return -1;
	}
	/*set i2c config timeout: us*/
	ioctl(fd,I2CIOC_TIMEOUT,100);

	addr = strtoul(argv[1], NULL, 16);
	val = strtoul(argv[2], NULL, 16);
	printf("cjc8988 i2c1 fd %d, set reg 0x%x to 0x%x\n",fd, addr, val);
	res = cjc8988_Write_Reg(fd, addr, val);
	printf("reg res %d\n", res);
	cjc8988_i2c_config_is_right(fd, addr, val);

	close(fd);
	return 0;
}

CONSOLE_CMD(reg, NULL, reg_set, CONSOLE_CMD_MODE_SELF,
	"reg addr val")

/* audio path: cvbs audio--> ADC --> i2si-->i2so 
 *	i2si : L01;
 *   start i2si
 */
int i2si_fd = -1, i2so_fd = -1;
static int cvbs_audio_open(int argc, char *argv[])
{
	struct snd_pcm_params i2si_params = {0};
	int channels = 2;

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
	struct adc_src_mode src_mode = {"linein"};
	ioctl(i2si_fd, SND_IOCTL_SET_CJC8988_INPUT, &src_mode);

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

	(void)argc;
	(void)argv;
	return 0;
}

static int mute_on(int argc, char *argv[])
{
	int fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0)
		return 0;

	ioctl(fd, SND_IOCTL_SET_MUTE, 1);

	close(fd);
	return 0;
}

static int mute_off(int argc, char *argv[])
{
	int fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0)
		return 0;

	ioctl(fd, SND_IOCTL_SET_MUTE, 0);

	close(fd);
	return 0;
}


CONSOLE_CMD(cvbs_on, NULL, cvbs_audio_open, CONSOLE_CMD_MODE_SELF,
	"cvbs_on")

CONSOLE_CMD(cvbs_off, NULL, cvbs_audio_close, CONSOLE_CMD_MODE_SELF,
	"cvbs_off")

CONSOLE_CMD(mute_on, NULL, mute_on, CONSOLE_CMD_MODE_SELF,
	"mute_on")
CONSOLE_CMD(mute_off, NULL, mute_off, CONSOLE_CMD_MODE_SELF,
	"mute_off")

#endif



