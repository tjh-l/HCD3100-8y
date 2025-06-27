/**
 * @file nds03_dev.c
 * @author tongsheng.tang
 * @brief NDS03 device setting functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */

#include "nds03_dev.h"
#include "nds03_def.h"
#include "nds03_comm.h"

/** SDK主版本 */
static uint8_t sdk_version_major = 2;
/** SDK次版本 */
static uint8_t sdk_version_minor = 0;
/** SDK小版本 */
static uint8_t sdk_version_patch = 0;


/**
 * @brief NDS03 Get SDK Version
 *        获取当前SDK的软件版本号
 * @return  uint32_t   
 * @retval  软件版本号
 */
uint32_t NDS03_GetSdkVersion(void)
{
    return ((uint32_t)sdk_version_major << 16) + ((uint32_t)sdk_version_minor << 8) + (uint32_t)sdk_version_patch;
}

/**
 * @brief NDS03 Get Firmware Version
 *        获取NDS03模组固件版本号
 * @param pNxDevice 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetFirmwareVersion(NDS03_Dev_t *pNxDevice)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadWord(pNxDevice, NDS03_REG_FW_VER, &pNxDevice->chip_info.fw_version);
    return ret;
}

/**
 * @brief NDS03 Get Therm
 *        获取NDS03的温度
 * @param pNxDevice 
 * @param therm     温度，单位为0.1度
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetTherm(NDS03_Dev_t *pNxDevice, int16_t* therm)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_THERM, (uint16_t*)therm);
    return ret;
}


/**
 * @brief NDS03 Set Pulse Num
 *        设置发光次数
 * @param pNxDevice 
 * @param pulse_num 发光次数
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetPulseNum(NDS03_Dev_t *pNxDevice, uint32_t pulse_num)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteWord(pNxDevice, NDS03_REG_PULSE_NUM, pulse_num);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);
    
    return ret;
}
/**
 * @brief NDS03 Get Pulse Num
 *        获取发光次数
 * @param pNxDevice 
 * @param pulse_num 发光次数
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetPulseNum(NDS03_Dev_t *pNxDevice, uint32_t *pulse_num)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadWord(pNxDevice, NDS03_REG_PULSE_NUM, pulse_num);
    
    return ret;
}

/**
 * @brief NDS03 Set Interval Time
 *        设置测量帧间隔时间(us)
 * @param pNxDevice 
 * @param inv_time  测量帧间隔时间
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetFrameTime(NDS03_Dev_t *pNxDevice, uint32_t frame_time_us)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteWord(pNxDevice, NDS03_REG_INV_TIME, frame_time_us);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);
    ret |= NDS03_ReadWord(pNxDevice, NDS03_REG_INV_TIME, &pNxDevice->config.range_frame_time_us);
    
    return ret;
}
/**
 * @brief NDS03 Get Interval Time
 *        获取测量间隔时间(us)
 * @param pNxDevice 
 * @param inv_time  测量间隔时间
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetFrameTime(NDS03_Dev_t *pNxDevice, uint32_t *frame_time_us)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadWord(pNxDevice, NDS03_REG_INV_TIME, &pNxDevice->config.range_frame_time_us);
    *frame_time_us = pNxDevice->config.range_frame_time_us;
    
    return ret;
}

/**
 * @brief NDS03 Set Confidence threshold
 *        配置置信度阈值 
 * @param pNxDevice 
 * @param confi_th 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetConfiTh(NDS03_Dev_t *pNxDevice, uint8_t confi_th)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CONFI_TH, confi_th);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);
    
    return ret;
}

/**
 * @brief NDS03 Get Confidence threshold
 *        获取置信度阈值
 * @param pNxDevice 
 * @param confi_th  置信度阈值
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetConfiTh(NDS03_Dev_t *pNxDevice, uint8_t *confi_th)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_CONFI_TH, confi_th);
    
    return ret;
}

/**
 * @brief NDS03 Set Target Num
 *        配置目标个数
 * @param pNxDevice 
 * @param num 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetTargetNum(NDS03_Dev_t *pNxDevice, uint8_t num)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    num = (num > NDS03_TARGET_MAX_NUM) ? NDS03_TARGET_MAX_NUM : num;
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_TARGET_NUM, num);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);
    if(ret == NDS03_ERROR_NONE){
        pNxDevice->config.target_num = num;
    }
    
    return ret;
}

/**
 * @brief NDS03 Get Target Num
 *        获取目标个数
 * @param pNxDevice 
 * @param num 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetTargetNum(NDS03_Dev_t *pNxDevice, uint8_t *num)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_TARGET_NUM, num);
    if(ret == NDS03_ERROR_NONE){
        pNxDevice->config.target_num = *num;
    }
    
    return ret;
}

/**
 * @brief NDS03 Set Gpio1 Configuration
 *        设置中断引脚的功能配置
 * @param pNxDevice: NDS03模组设备信息结构体
 * @param functionality: 中断功能设置
 *            NDS03_GPIO1_FUNCTIONALITY_OFF: 无功能（默认）
 *            NDS03_GPIO1_THRESHOLD_LOW：比较功能，当深度小于低阈值时，GPIO输出有效电平
 *            NDS03_GPIO1_THRESHOLD_HIGH：比较功能，当深度大于高阈值时，GPIO输出有效电平
 *            NDS03_GPIO1_THRESHOLD_DOMAIN_OUT：比较功能，当深度小于低阈值或者大于高阈值时，GPIO输出有效电平
 *            NDS03_GPIO1_NEW_MEASURE_READY:深度数据有效功能，当深度数据有效时，GPIO输出有效电平
 * @param polarity: INT引脚有效电平
 *            NDS03_GPIO1_POLARITY_LOW：低电平有效
 *            NDS03_GPIO1_POLARITY_HIGH：高电平有效
 * @return NDS03_Error
*/
NDS03_Error NDS03_SetGpio1Config(NDS03_Dev_t *pNxDevice, NDS03_Gpio1Func_t functionality, NDS03_Gpio1Polar_t polarity)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         rbuf;

    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_GPIO1_FUNC, &rbuf);
    /** 设置中断功能 */
    rbuf = rbuf & (~NDS03_GPIO1_FUNCTIONALITY_MASK);
    rbuf = rbuf | (functionality & NDS03_GPIO1_FUNCTIONALITY_MASK);
    /** 设置中断引脚极性 */
    if(polarity == NDS03_GPIO1_POLARITY_HIGH)
        rbuf = rbuf | NDS03_GPIO1_POLARITY_MASK;
    else
        rbuf = rbuf & (~NDS03_GPIO1_POLARITY_MASK);

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_GPIO1_FUNC, rbuf);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);

    return ret;
}

/** 
 * @brief NDS03 Get Gpio1 Config
 *        获取中断引脚的功能配置
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param functionality: 获取到的中断功能变量指针
 * @param polarity: 获取到的中断引脚极性变量指针
 * @return NDS03_Error
*/
NDS03_Error NDS03_GetGpio1Config(NDS03_Dev_t *pNxDevice, NDS03_Gpio1Func_t *functionality, NDS03_Gpio1Polar_t *polarity)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         rbuf;

    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_GPIO1_FUNC, &rbuf);

    if(NDS03_ERROR_NONE == ret)
    {
        // Polarity
        *polarity = ((rbuf & NDS03_GPIO1_POLARITY_MASK) == NDS03_GPIO1_POLARITY_MASK) ? NDS03_GPIO1_POLARITY_HIGH : NDS03_GPIO1_POLARITY_LOW;
        // Functionality
        *functionality = rbuf & NDS03_GPIO1_FUNCTIONALITY_MASK;
    }
    
    return ret;
}

/**
 * @brief NDS03 Set Depth Threshold
 *        设置深度阈值
 *        该功能仅用于GPIO功能为
 *        NDS03_GPIO1_THRESHOLD_LOW
 *        或NDS03_GPIO1_THRESHOLD_HIGH
 *        或NDS03_GPIO1_THRESHOLD_DOMAIN_OUT
 * @param pNxDevice: NDS03模组设备信息结构体
 * @param depth_low:   低深度阈值 / mm
 * @param depth_high:  高深度阈值 / mm
 * @return NDS03_Error
*/
NDS03_Error NDS03_SetDepthThreshold(NDS03_Dev_t *pNxDevice, uint16_t depth_low, uint16_t depth_high)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_DEPTH_TH_L, depth_low);
    ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_DEPTH_TH_H, depth_high);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);

    return ret;
}

/**
 * @brief NDS03 Get Depth Threshold
 *        获取深度阈值
 * @param pNxDevice: NDS03模组设备信息结构体
 * @param depth_low:   低深度阈值 / mm
 * @param depth_high:  高深度阈值 / mm
 * @return NDS03_Error
*/
NDS03_Error NDS03_GetDepthThreshold(NDS03_Dev_t *pNxDevice, uint16_t *depth_low, uint16_t *depth_high)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_DEPTH_TH_L, depth_low);
    ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_DEPTH_TH_H, depth_high);

    return ret;
}

/**
 * @brief NDS03 Waitfor Data Val
 *        等待数据完成
 * @param pNxDevice     模组设备
 * @param flag          读命令标志位
 * @return NDS03_Error 
 */
NDS03_Error NDS03_WaitforDataVal(NDS03_Dev_t *pNxDevice, uint8_t flag, int32_t timeout_ms)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint16_t        rbuf;
    uint8_t         dat_valid_flag = 0x00;
    int32_t         retry_cnt;

    retry_cnt = timeout_ms*2;
    do{
        ret = NDS03_ReadHalfWord(pNxDevice, NDS03_REG_DAT_VAL, &rbuf);
        dat_valid_flag = rbuf&0x00FF;
        if(ret != NDS03_ERROR_NONE)
        {
            return NDS03_ERROR_I2C;
        }
        if(dat_valid_flag == flag)
        {
            break;
        }
        NDS03_Delay10us(pNxDevice, 50);
    }while(--retry_cnt);

    if(retry_cnt == 0)
    {
        NX_PRINTF("data_valid_flag: %d\r\n", dat_valid_flag);
        ret = NDS03_ERROR_TIMEOUT;
    }

    return ret;
}

/**
 * @brief NDS03 Waitfor Cmd Val
 *        等待命令完成
 * @param pNxDevice     模组设备
 * @param cmd           命令
 * @param timeout_ms    超时时间
 * @return NDS03_Error 
 */
NDS03_Error NDS03_WaitforCmdVal(NDS03_Dev_t *pNxDevice, uint8_t cmd, int32_t timeout_ms)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    int32_t         retry_cnt;
    uint8_t         val;

    retry_cnt = timeout_ms*2;
    do{
        ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_CMD_VAL, &val);
        if(val == cmd)
        {
            break;
        }
        NDS03_Delay10us(pNxDevice, 50);
    }while(--retry_cnt);

    if(retry_cnt == 0)
    {
        NX_PRINTF("Timeout!!, val: 0x%02x\r\n", val);
        ret = NDS03_ERROR_TIMEOUT;
    }

    return ret;
}

/**
 * @brief NDS03 Read User Data
 *        读取NDS03中用户数据区域，支持任意地址循环读取，比如从0xFFF0开始读取32个字节，
 *        则前16个字节数据是0xFFF0~0xFFFF的数据，后16个字节是0x0000~0x000F的数据
 * @param pNxDevice     模组设备
 * @param addr          数据地址，0x0000~0xFFFF
 * @param rbuf          数据指针
 * @param size          数据大小
 * @return NDS03_Error 
 */
NDS03_Error NDS03_ReadUserData(NDS03_Dev_t *pNxDevice, uint16_t addr, uint8_t *rbuf, uint32_t size)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint32_t        rsize, _size;
    uint8_t         one_size;

    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_CACHE_SIZE, &one_size);
    _size = (uint32_t)one_size;
    rsize = 0;
    while((rsize < size) && (ret == NDS03_ERROR_NONE)){
        ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_CACHE_ADDR, addr);
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_DAT_REQ, NDS03_USER_DATA_FLAG);
        ret |= NDS03_WaitforDataVal(pNxDevice, NDS03_USER_DATA_FLAG, 200);
        ret |= NDS03_ReadNBytes(pNxDevice, NDS03_REG_CACHE_DATA, rbuf, ((size-rsize)>_size) ? _size:(size-rsize));
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_DAT_VAL, NDS03_DATA_VAL_IDLE);
        rbuf += _size;
        addr += _size;
        rsize += _size;
    }
    
    return ret;
}

/**
 * @brief NDS03 Read User Data
 *        写入NDS03中用户数据区域, 支持任意地址循环写入，比如从0xFFF0开始写入32个字节，
 *        则前16个字节数据是0xFFF0~0xFFFF的数据，后16个字节是0x0000~0x000F的数据
 * @param pNxDevice     模组设备
 * @param addr          数据地址，0x0000~0xFFFF
 * @param wbuf          数据指针
 * @param size          数据大小
 * @return NDS03_Error 
 */
NDS03_Error NDS03_WriteUserData(NDS03_Dev_t *pNxDevice, uint16_t addr, uint8_t *wbuf, uint32_t size)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint32_t        rsize, _size;
    uint8_t         one_size;

    // NX_PRINTF("%s Start\r\n", __func__);
    ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_CACHE_SIZE, &one_size);
    _size = (uint32_t)one_size;
    rsize = 0;
    while((rsize < size) && (ret == NDS03_ERROR_NONE)){
        ret |= NDS03_WriteNBytes(pNxDevice, NDS03_REG_CACHE_DATA, wbuf, ((size-rsize)>_size) ? _size:(size-rsize));
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_DATA_SIZE, ((size-rsize)>_size) ? _size:(size-rsize));
        ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_CACHE_ADDR, addr);
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_ENA, NDS03_CMD_WRITE_USER_DATA_ENA);
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_REQ, NDS03_CMD_WRITE_USER_DATA);
        ret |= NDS03_WaitforCmdVal(pNxDevice, NDS03_CMD_WRITE_USER_DATA, 500);
        ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_VAL, NDS03_CMD_VAL_IDLE);
        wbuf += _size;
        addr += _size;
        rsize += _size;
    }
    // NX_PRINTF("%s End\r\n", __func__);
    
    return ret;
}

/**
 * @brief NDS03 Software Sleep
 *          软件睡眠
 * @param   pNxDevice       模组设备
 * @param   sleep_time_ms   软件睡眠时间，达到时间后自动唤醒
 * @return  NDS03_Error 
 */
NDS03_Error NDS03_SoftSleep(NDS03_Dev_t *pNxDevice, uint16_t sleep_time_ms){
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_SLEEP_TIME, sleep_time_ms);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_SLEEP_MODE, NDS03_SOFTWARE_SLEEP);

    return ret;
}

/**
 * @brief NDS03 Sleep
 *        NDS03进入休眠
 * @param pNxDevice   模组设备
 * @return NDS03_Error 
 */
NDS03_Error NDS03_Sleep(NDS03_Dev_t *pNxDevice){
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_SetXShutPinLevel(pNxDevice, 0);
    return ret;
}

/**
 * @brief NDS03 Wakeup
 *        NDS03从睡眠中唤醒
 * @param pNxDevice   模组设备
 * @return NDS03_Error 
 */
NDS03_Error NDS03_Wakeup(NDS03_Dev_t *pNxDevice){
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_SetXShutPinLevel(pNxDevice, 1);
    return ret;
}

/**
 * @brief NDS03 Set Device Address
 *        设置模组设备地址
 * @param pNxDevice   模组设备
 * @param dev_addr 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetDevAddr(NDS03_Dev_t *pNxDevice, uint8_t dev_addr)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_DEV_ADDR, dev_addr);
    if(ret == NDS03_ERROR_NONE)
    {
        pNxDevice->platform.i2c_dev_addr = dev_addr;
    }
    return ret;
}

/**
 * @brief NDS03 InitDevice
 *        初始化设备
 * @param   pNxDevice   模组设备
 * @return  NDS03_Error
 */
NDS03_Error NDS03_InitDevice(NDS03_Dev_t *pNxDevice)
{
	NDS03_Error     ret = NDS03_ERROR_NONE;

    // NX_PRINTF("%s Start!\r\n", __func__);

    ret |= NDS03_GetFirmwareVersion(pNxDevice);
	ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_DATA_CNT, &pNxDevice->data_cnt);
	ret |= NDS03_GetFrameTime(pNxDevice, &pNxDevice->config.range_frame_time_us);
    pNxDevice->config.continuous_flag = 0;

    // NX_PRINTF("%s End!\r\n", __func__);

	return ret;
}

/**
 * @brief NDS03 Wait for Device Boot Up 
 *        等待NDS03模组启动
 * @param   pNxDevice   模组设备
 * @return  NDS03_ERROR_NONE:成功
 *          NDS03_ERROR_BOOT:启动失败--请检测模组是否焊接好，还有i2c地址与读写函数是否错误。
 *          NDS03_ERROR_FW:固件不兼容--请与FAE联系，是否模组的固件与SDK不兼容。
 */
NDS03_Error NDS03_WaitDeviceBootUp(NDS03_Dev_t *pNxDevice)
{ 
    NDS03_Error     ret = NDS03_ERROR_NONE;
    int32_t         try_times = 200;
    uint8_t         slave_addr = pNxDevice->platform.i2c_dev_addr;

    ret |= NDS03_SetXShutPinLevel(pNxDevice, 0);
    NDS03_Delay10us(pNxDevice, 20);
    ret |= NDS03_SetXShutPinLevel(pNxDevice, 1);
    NDS03_Delay10us(pNxDevice, 200);
    pNxDevice->platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR;
    pNxDevice->data_cnt     = 0;
    pNxDevice->config.continuous_flag = 0;
    do{
        NDS03_Delay10us(pNxDevice, 10);
        ret |= NDS03_ReadByte(pNxDevice, NDS03_REG_STATE, &pNxDevice->dev_pwr_state);
    }while((pNxDevice->dev_pwr_state != NDS03_STATE_SOFT_READY) && \
            (pNxDevice->dev_pwr_state != NDS03_STATE_GOT_DEPTH) && --try_times);
	if(NDS03_ERROR_NONE != ret)
    {
        NX_PRINTF("NDS03 i2c error\r\n");
        return NDS03_ERROR_I2C;
    }
	if(0 == try_times)
    {
        NX_PRINTF("state: 0x%02x\r\n", pNxDevice->dev_pwr_state);
        NX_PRINTF("NDS03 boot error\r\n");
        return NDS03_ERROR_BOOT;
    }

    if(slave_addr != pNxDevice->platform.i2c_dev_addr)
    {
        ret |= NDS03_SetDevAddr(pNxDevice, slave_addr);
    }

    return ret;
}

