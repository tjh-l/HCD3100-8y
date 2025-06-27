/**
 * @file nds03_comm.h
 * @author tongsheng.tang
 * @brief NDS03 communication functions
 * @version 1.x.x
 * @date 2021-11
 * 
 * @copyright Copyright (c) 2021, Shenzhen Nephotonics Inc.
 * 
 */
#ifndef __NDS03_DATA_H__
#define __NDS03_DATA_H__

#include "nds03_def.h"
/** @defgroup NDS03_Data_Group NDS03 Data Funtions
 *  @brief NDS03 Data Funtions
 *  @{
 */

/** 检测NDS03测距是否完成 */
NDS03_Error NDS03_GetRangingDataReady(NDS03_Dev_t *pNxDevice);
/** 发送开始单词测量信号 */
NDS03_Error NDS03_StartSingleMeasurement(NDS03_Dev_t *pNxDevice);
/** 发送开始连续测量信号 */
NDS03_Error NDS03_StartContinuousMeasurement(NDS03_Dev_t *pNxDevice);
/** 发送结束连续测量信号 */
NDS03_Error NDS03_StopContinuousMeasurement(NDS03_Dev_t *pNxDevice);
/** 清除数据有效位 */
NDS03_Error NDS03_ClearDataValidFlag(NDS03_Dev_t *pNxDevice);
/** 读取深度和幅度值 */
NDS03_Error NDS03_ReadRangingData(NDS03_Dev_t *pNxDevice);
/** 获取连续测量深度和幅度值 */
NDS03_Error NDS03_GetContinuousRangingData(NDS03_Dev_t *pNxDevice);
/** 获取一次测量深度和幅度值 */
NDS03_Error NDS03_GetSingleRangingData(NDS03_Dev_t *pNxDevice);
/** 获取一次中断数据 */
NDS03_Error NDS03_GetInterruptRangingData(NDS03_Dev_t *pNxDevice);

/** @} NDS03_Data_Group */

#endif // __NDS03_DATA_H__
