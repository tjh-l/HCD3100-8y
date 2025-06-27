/**
 * @file nds03_dev.h
 * @author tongsheng.tang
 * @brief NDS03 device setting functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */
#ifndef __NDS03_DEV_H__
#define __NDS03_DEV_H__

#include "nds03_def.h"

/** @defgroup NDS03_Dev_Group NDS03 Device Funtions
 *  @brief NDS03 Device Funtions
 *  @{
 */

/** 获取SDK版本号 */
uint32_t NDS03_GetSdkVersion(void);
/** 获取NDS03模组固件版本号 */
NDS03_Error NDS03_GetFirmwareVersion(NDS03_Dev_t *pNxDevice);
/** 获取温度 */
NDS03_Error NDS03_GetTherm(NDS03_Dev_t *pNxDevice, int16_t* therm);
/** 设置发光次数 */
NDS03_Error NDS03_SetPulseNum(NDS03_Dev_t *pNxDevice, uint32_t pulse_num);
/** 获取发光次数 */
NDS03_Error NDS03_GetPulseNum(NDS03_Dev_t *pNxDevice, uint32_t *pulse_num);
/** 设置测量间隔时间 */
NDS03_Error NDS03_SetFrameTime(NDS03_Dev_t *pNxDevice, uint32_t frame_time_us);
/** 获取测量间隔时间 */
NDS03_Error NDS03_GetFrameTime(NDS03_Dev_t *pNxDevice, uint32_t *frame_time_us);
/** 设置置信度阈值 */
NDS03_Error NDS03_SetConfiTh(NDS03_Dev_t *pNxDevice, uint8_t confi_th);
/** 获取置信度阈值 */
NDS03_Error NDS03_GetConfiTh(NDS03_Dev_t *pNxDevice, uint8_t *confi_th);
/** 设置目标个数 */
NDS03_Error NDS03_SetTargetNum(NDS03_Dev_t *pNxDevice, uint8_t num);
/** 获取目标个数 */
NDS03_Error NDS03_GetTargetNum(NDS03_Dev_t *pNxDevice, uint8_t *num);
/** 设置中断引脚功能 */
NDS03_Error NDS03_SetGpio1Config(NDS03_Dev_t *pNxDevice, NDS03_Gpio1Func_t functionality, NDS03_Gpio1Polar_t polarity);
/** 获取中断引脚功能 */
NDS03_Error NDS03_GetGpio1Config(NDS03_Dev_t *pNxDevice, NDS03_Gpio1Func_t *functionality, NDS03_Gpio1Polar_t *polarity);
/** 设置深度阈值 */
NDS03_Error NDS03_SetDepthThreshold(NDS03_Dev_t *pNxDevice, uint16_t depth_low, uint16_t depth_high);
/** 获取深度阈值 */
NDS03_Error NDS03_GetDepthThreshold(NDS03_Dev_t *pNxDevice, uint16_t *depth_low, uint16_t *depth_high);
/** 等待获取数据结束 */
NDS03_Error NDS03_WaitforDataVal(NDS03_Dev_t *pNxDevice, uint8_t flag, int32_t timeout_ms);
/** 等待命令结束 */
NDS03_Error NDS03_WaitforCmdVal(NDS03_Dev_t *pNxDevice, uint8_t cmd, int32_t timeout_ms);
/** 从NDS03中读取用户数据 */
NDS03_Error NDS03_ReadUserData(NDS03_Dev_t *pNxDevice, uint16_t addr, uint8_t *rbuf, uint32_t size);
/** 从NDS03中写入用户数据 */
NDS03_Error NDS03_WriteUserData(NDS03_Dev_t *pNxDevice, uint16_t addr, uint8_t *wbuf, uint32_t size);
/** NDS03进入软件睡眠 */
NDS03_Error NDS03_SoftSleep(NDS03_Dev_t *pNxDevice, uint16_t sleep_time_ms);
/** NDS03进入睡眠 */
NDS03_Error NDS03_Sleep(NDS03_Dev_t *pNxDevice);
/** NDS03从睡眠中唤醒 */
NDS03_Error NDS03_Wakeup(NDS03_Dev_t *pNxDevice);
/** 设置模组设备地址 */
NDS03_Error NDS03_SetDevAddr(NDS03_Dev_t *pNxDevice, uint8_t dev_addr);
/** 初始化设备 */
NDS03_Error NDS03_InitDevice(NDS03_Dev_t *pNxDevice);
/** 等待设备启动 */
NDS03_Error NDS03_WaitDeviceBootUp(NDS03_Dev_t *pNxDevice);

/** @} NDS03_Dev_Group */

#endif

