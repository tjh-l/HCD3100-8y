#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/ld.h>
#include <linux/jiffies.h>
#include <stdio.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <hcuapi/gpio.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>
#include <freertos/semphr.h>

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <kernel/lib/fdt_api.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>

/***************************************************************
文件名		: ft5x06.c
版本	   	: V1.0
描述	   	: FT5X06，包括FT5206、FT5426等触摸屏驱动程序
***************************************************************/

#define MAX_SUPPORT_POINTS 5 /* 5点触摸 	*/
#define TOUCH_EVENT_DOWN 0x00 /* 按下 	*/
#define TOUCH_EVENT_UP 0x01 /* 抬起 	*/
#define TOUCH_EVENT_ON 0x02 /* 接触 	*/
#define TOUCH_EVENT_RESERVED 0x03 /* 保留 	*/

/* FT5X06寄存器相关宏定义 */
#define FT5X06_TD_STATUS_REG 0X02 /*	状态寄存器地址 		*/
#define FT5x06_DEVICE_MODE_REG 0X00 /* 模式寄存器 			*/
#define FT5426_IDG_MODE_REG 0XA4 /* 中断模式				*/

#define FTS_ONE_TCH_LEN                     6
#define FTS_TOUCH_DATA_LEN (MAX_SUPPORT_POINTS * FTS_ONE_TCH_LEN + 3)

#define RESOLUTION_X 1920
#define RESOLUTION_Y 1200

#define msleep(x) usleep(x * 1000)

struct ft5x06_dev {
    struct input_dev *input; /* input结构体 		*/
    struct i2c_client *client; /* I2C客户端 		*/
    unsigned int addr;
    const char *i2c_devpath;
    unsigned int irq; /* 中断IO		*/
    unsigned int reset; /* 复位IO	 */
    unsigned int x_max;
    unsigned int y_max;
    struct work_s work;
};

struct ft5x06_dev *ft5x06;
struct i2c_client *client = NULL;

/*
 * @description     : 复位FT5X06
 * @param - client 	: 要操作的i2c
 * @param - multidev: 自定义的multitouch设备
 * @return          : 0，成功;其他负值,失败
 */
static int ft5x06_ts_reset(struct i2c_client *client, struct ft5x06_dev *dev)
{
    gpio_configure(dev->reset, GPIO_DIR_OUTPUT);
    gpio_set_output(dev->reset, 0);

    msleep(5);
    gpio_set_output(dev->reset, 1); /* 输出高电平，停止复位 */
    msleep(300);

    return 0;
}

/*
 * @description	: 从FT5X06读取多个寄存器数据
 * @param - dev:  ft5x06设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return 		: 操作结果
 */
static int ft5x06_read_regs(struct ft5x06_dev *dev, u8 reg, void *val, int len)
{
    s32 ret = -1;
    int retries = 0;
    int fd = open(dev->i2c_devpath, O_RDWR);
    if (fd < 0) {
        printf("open i2c fail\n");
        return -1;
    }

    struct i2c_transfer_s xfer_read;
    struct i2c_msg_s msgs[2] = { 0 };

    //	printf("\n---ft5x06_read_regs, addr=0x%x\n",(uint8_t)dev->addr);

    msgs[0].flags = 0x0;
    msgs[0].addr = (uint8_t)dev->addr;
    msgs[0].length = 1;
    msgs[0].buffer = &reg;

    msgs[1].flags = 0x1;
    msgs[1].addr = (uint8_t)dev->addr;
    msgs[1].length = len;
    msgs[1].buffer = val;

    xfer_read.msgv = msgs;
    xfer_read.msgc = 2;

    while (retries < 5) {
        ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_read);
        if (ret == 2)
            break;
        retries++;
    }
    close(fd);
    //printf("\n---ft5x06_read_regs, ret =%d\n",ret);
    return ret;
}

/*
 * @description	: 向ft5x06多个寄存器写入数据
 * @param - dev:  ft5x06设备
 * @param - reg:  要写入的寄存器首地址
 * @param - val:  要写入的数据缓冲区
 * @param - len:  要写入的数据长度
 * @return 	  :   操作结果
 */
static s32 ft5x06_write_regs(struct ft5x06_dev *dev, u8 reg, u8 *buf, u8 len)
{
    struct i2c_transfer_s xfer_write;
    struct i2c_msg_s msg;
    s32 ret = -1;
    s32 retries = 0;

    int fd = open(dev->i2c_devpath, O_RDWR);
    if (fd < 0) {
        printf("open i2c fail\n");
        return -1;
    }

    msg.flags = !0x1;
    msg.addr = (uint8_t)dev->addr;
    msg.length = len;
    msg.buffer = buf;

    xfer_write.msgv = &msg;
    xfer_write.msgc = 1;

    while (retries < 5) {
        ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
        if (ret == 1)
            break;
        retries++;
    }

    close(fd);
}

/*
 * @description	: 向ft5x06指定寄存器写入指定的值，写一个寄存器
 * @param - dev:  ft5x06设备
 * @param - reg:  要写的寄存器
 * @param - data: 要写入的值
 * @return   :    无
 */
static void ft5x06_write_reg(struct ft5x06_dev *dev, u8 reg, u8 data)
{
    u8 buf = data;
    ft5x06_write_regs(dev, reg, &buf, 1);
}

/*
 * @description     : FT5X06中断服务函数
 * @param - irq 	: 中断号
 * @param - dev_id	: 设备结构。
 * @return 			: 中断执行结果
 */
static void __ft5x06_handler(void *dev_id)
{
    struct ft5x06_dev *multidata = (struct ft5x06_dev *)dev_id;

    u8 rdbuf[FTS_TOUCH_DATA_LEN];
    int i, type, x, y, id;
    int offset, tplen;
    //int ret=0;
    bool down;

    offset = 1; /* 偏移1，也就是0X02+1=0x03,从0X03开始是触摸值 */
    tplen = 6; /* 一个触摸点有6个寄存器来保存触摸值 */

    memset(rdbuf, 0, sizeof(rdbuf)); /* 清除 */

    /* 读取FT5X06触摸点坐标从0X02寄存器开始，连续读取29个寄存器 */
    ft5x06_read_regs(multidata, FT5X06_TD_STATUS_REG, rdbuf, FTS_TOUCH_DATA_LEN);

    /* 上报每一个触摸点坐标 */
    for (i = 0; i < MAX_SUPPORT_POINTS; i++) {
        u8 *buf = &rdbuf[i * tplen + offset];

        /* 以第一个触摸点为例，寄存器TOUCH1_XH(地址0X03),各位描述如下：
         * bit7:6  Event flag  0:按下 1:释放 2：接触 3：没有事件
         * bit5:4  保留
         * bit3:0  X轴触摸点的11~8位。
         */
        type = buf[0] >> 6; /* 获取触摸类型 */
        if (type == TOUCH_EVENT_RESERVED)
            continue;

        /* 触摸屏坐标 */
        x = ((buf[0] << 8) | buf[1]) & 0x0fff;
        y = ((buf[2] << 8) | buf[3]) & 0x0fff;

        /* 以第一个触摸点为例，寄存器TOUCH1_YH(地址0X05),各位描述如下：
         * bit7:4  Touch ID  触摸ID，表示是哪个触摸点

         * bit3:0  Y轴触摸点的11~8位。
         */
        id = (buf[2] >> 4) & 0x0f;

        down = type != TOUCH_EVENT_UP;
        /**
         *  如果 type 不等于 TOUCH_EVENT_UP，则 down 的取值为真（true）。
         *  如果 type 等于 TOUCH_EVENT_UP，则 down 的取值为假（false）。
         *  在这行代码中，down 变量可能被用来表示触摸事件是否为按下事件（即非抬起事件）。
         *  这种逻辑在处理触摸事件时常见，通常用于区分不同类型的触摸事件，
         *  比如按下、移动、抬起等，以便程序可以相应地进行处理
         **/
        //printf("[%d] %s (%d,%d)\n", id, down ? "down" : "up", x, y);

        input_mt_slot(multidata->input, id);
        input_mt_report_slot_state(multidata->input, MT_TOOL_FINGER, down);

        if (!down)
            continue;

        input_report_abs(multidata->input, ABS_MT_POSITION_X, x);
        input_report_abs(multidata->input, ABS_MT_POSITION_Y, y);
    }

    // input_mt_report_pointer_emulation(multidata->input, true);
    input_mt_sync_frame(multidata->input);
    input_sync(multidata->input);

    gpio_irq_enable(multidata->irq);
}

static void ft5x06_handler(uint32_t dev_id)
{
    struct ft5x06_dev *multidata = (struct ft5x06_dev *)dev_id;

    if (work_available(&multidata->work)) {
	    gpio_irq_disable(multidata->irq);
	    work_queue(HPWORK, &multidata->work, __ft5x06_handler, (void *)dev_id, 0);
    }
}

/*
 * @description     : FT5x06中断初始化
 * @param - client 	: 要操作的i2c
 * @param - multidev: 自定义的multitouch设备
 * @return          : 0，成功;其他负值,失败
 */
static int ft5x06_ts_irq(struct i2c_client *client, struct ft5x06_dev *dev)
{
    int ret = 0;

    gpio_configure(dev->irq, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
    ret = gpio_irq_request(dev->irq, ft5x06_handler, (uint32_t)dev);
    if (ret < 0) {
        printf("ERROR: gpio_irq_request() failed: %d\n", ret);
        return ret;
    }

    return 0;
}

/*
  * @description     : i2c驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : i2c设备
  * @param - id      : i2c设备ID
  * @return          : 0，成功;其他负值,失败
  */
static int ft5x06_ts_probe(void)
{
    int ret, np;
    struct input_dev *input_dev;

    ft5x06 = kzalloc(sizeof(*ft5x06), GFP_KERNEL);
    ft5x06->x_max = RESOLUTION_X;
    ft5x06->y_max = RESOLUTION_Y;
    client = ft5x06->client;

    np = fdt_get_node_offset_by_path("/hcrtos/ft5x06_ts@38");
    if (np < 0) {
        printf("no find ft5x06_ts\n");
        ret = -ENOMEM;
    }

    if (fdt_get_property_u_32_index(np, "reset-gpio", 0, (u32 *)&ft5x06->reset))
        return -1;
    if (fdt_get_property_u_32_index(np, "irq-gpio", 0, (u32 *)&ft5x06->irq))
        return -1;
    if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&ft5x06->addr))
        return -1;
    if (fdt_get_property_string_index(np, "i2c-devpath", 0, &ft5x06->i2c_devpath))
        return -1;

    /* 1，复位FT5x06 */
    ret = ft5x06_ts_reset(client, ft5x06);
    if (ret < 0) {
        printf("\n---ft5x06_ts_reset fail~~ \n");
        goto fail;
    }

    /* 2，初始化FT5X06 */
    ft5x06_write_reg(ft5x06, FT5x06_DEVICE_MODE_REG, 0); /* 进入正常模式 	*/
    ft5x06_write_reg(ft5x06, FT5426_IDG_MODE_REG, 1); /* FT5426中断模式	*/

    /* 3，input设备注册 */
    input_dev = input_allocate_device();
    if (!input_dev) {
        ret = -ENOMEM;
        goto fail;
    }

    input_dev->name = "ft5x06";
    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(BTN_TOUCH, input_dev->keybit);

    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0XFF, 0, 0);
    input_set_abs_params(input_dev, ABS_X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(input_dev, ABS_Y, 0, RESOLUTION_Y, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, RESOLUTION_Y, 0, 0);

    ft5x06->input = input_dev;

    ret = input_mt_init_slots(input_dev, MAX_SUPPORT_POINTS, 0);
    if (ret) {
        goto fail;
    }

    ret = input_register_device(input_dev);
    if (ret) {
        printf("\nRegister %s input device failed\n", input_dev->name);
        goto fail;
    }

    /* 4，初始化中断 */
    ret = ft5x06_ts_irq(client, ft5x06);
    if (ret < 0) {
        printf("\n---ft5x06_ts_irq fail~~ \n");
        goto fail;
    }

    return 0;

fail:
    return ret;
}

static int hc_ft5426_init(void)
{
    return ft5x06_ts_probe();
}

module_driver(hc_ft5426_driver, hc_ft5426_init, NULL, 1)
