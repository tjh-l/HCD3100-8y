/**
 * @file nds03_calib.h
 * @author tongsheng.tang
 * @brief NDS03 Calibration functions
 * @version 1.x.x
 * @date 2023-04-23
 * 
 * @copyright Copyright (c) 2021, Shenzhen Nephotonics Inc.
 * 
 */

#ifndef __NDS03_CALIB_H__
#define __NDS03_CALIB_H__

#include "nds03_def.h"

/** @defgroup NDS03_Calibration_Group NDS03 Calibration Funtions
 *  @brief NDS03 Calibration Funtions
 *  @{
 */

/** 获取offset标定距离 */
NDS03_Error NDS03_GetOffsetCalibDepthMM(NDS03_Dev_t *pNxDevice, uint16_t *calib_depth_mm);
/** 设置offset标定距离 */
NDS03_Error NDS03_SetOffsetCalibDepthMM(NDS03_Dev_t *pNxDevice, uint16_t calib_depth_mm);
/** Offset标定函数带有标定深度设置 */
NDS03_Error NDS03_OffsetCalibration(NDS03_Dev_t *pNxDevice);
/** Offset标定函数带有标定深度设置 */
NDS03_Error NDS03_OffsetCalibrationAtDepth(NDS03_Dev_t *pNxDevice, uint16_t calib_depth_mm);
/** XTalk标定 */
NDS03_Error NDS03_XtalkCalibration(NDS03_Dev_t *pNxDevice);
/** 获取标定串扰值 */
NDS03_Error NDS03_GetXTalkValue(NDS03_Dev_t *pNxDevice, uint16_t* xtalk_value);

/** @} NDS03_Calibration_Group */

#endif

