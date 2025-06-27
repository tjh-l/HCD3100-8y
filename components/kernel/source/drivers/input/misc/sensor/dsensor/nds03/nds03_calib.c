/**
 * @file nds03_calib.c
 * @author tongsheng.tang
 * @brief NDS03 Calibration functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */

#include "nds03_comm.h"
#include "nds03_dev.h"
#include "nds03_data.h"
#include "nds03_calib.h"


/**
 * @brief NDS03 Get Offset Calib Depth MM
 *        获取offset标定距离
 * @param pNxDevice 
 * @param calib_depth_mm 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetOffsetCalibDepthMM(NDS03_Dev_t *pNxDevice, uint16_t *calib_depth_mm)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_OFFSET_MM, calib_depth_mm);

    return ret;
}

/**
 * @brief NDS03 Set Offset Calib Depth MM
 *        设置offset标定距离
 * @param pNxDevice 
 * @param calib_depth_mm 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_SetOffsetCalibDepthMM(NDS03_Dev_t *pNxDevice, uint16_t calib_depth_mm)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    if(calib_depth_mm == 0)
    {
        ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_OFFSET_MM, &calib_depth_mm);
    }
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteHalfWord(pNxDevice, NDS03_REG_OFFSET_MM, calib_depth_mm);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CFG_ENA, NDS03_CMD_ENA_DISABLE);

    return ret;
}

/**
 * @brief    ToF Offset 标定
 * @details  不可以指定标定距离，使用默认距离，如果没有修改，那值默认是500mm
 * 
 * @param   pNxDevice       设备模组
 * @return  int8_t  
 * @retval  0:  成功
 * @retval  !0: Offset标定失败
 */
NDS03_Error NDS03_OffsetCalibration(NDS03_Dev_t *pNxDevice)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    NX_PRINTF("%s Start!\r\n", __func__);

    // 打开使能
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_REQ, NDS03_CMD_OFFSET_CALIB);
    ret |= NDS03_WaitforCmdVal(pNxDevice, NDS03_CMD_OFFSET_CALIB, 5000);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_VAL, NDS03_CMD_ENA_DISABLE);
    if(ret == NDS03_ERROR_NONE)
        NDS03_ReadByte(pNxDevice, NDS03_REG_CALIB_STATE, (uint8_t*)&ret);
    NX_PRINTF("%s End!\r\n", __func__);

    return ret;
}


/**
 * @brief    ToF Offset 标定
 * @details  可以指定标定距离
 * 
 * @param   pNxDevice       设备模组
 * @param   calib_depth_mm  标定距离
 * @return  int8_t  
 * @retval  0:  成功
 * @retval  !0: Offset标定失败
 */
NDS03_Error NDS03_OffsetCalibrationAtDepth(NDS03_Dev_t *pNxDevice, uint16_t calib_depth_mm)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    NX_PRINTF("%s Start!\r\n", __func__);
    ret |= NDS03_SetOffsetCalibDepthMM(pNxDevice, calib_depth_mm);
    // 打开使能
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_REQ, NDS03_CMD_OFFSET_CALIB);
    ret |= NDS03_WaitforCmdVal(pNxDevice, NDS03_CMD_OFFSET_CALIB, 5000);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_VAL, NDS03_CMD_ENA_DISABLE);
    if(ret == NDS03_ERROR_NONE)
    {
        NDS03_ReadByte(pNxDevice, NDS03_REG_CALIB_STATE, (uint8_t*)&ret);
        ret = ret & NDS03_CALIB_ERROR_OFFSET;
    }
    NX_PRINTF("%s End!\r\n", __func__);

    return ret;
}

/**
 * @brief   NDS03 Xtalk Calibration
 *          NDS03串扰/盖板标定
 * @param   pNxDevice       设备模组
 * @return  int8_t  
 * @retval  0:  成功
 * @retval  !0: xtalk标定失败
 */
NDS03_Error NDS03_XtalkCalibration(NDS03_Dev_t *pNxDevice)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    NX_PRINTF("%s Start!\r\n", __func__);
    // 打开使能
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_ENA, NDS03_CMD_ENA_ENABLE);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_REQ, NDS03_CMD_XTALK_CALIB);
    ret |= NDS03_WaitforCmdVal(pNxDevice, NDS03_CMD_XTALK_CALIB, 5000);
    ret |= NDS03_WriteByte(pNxDevice, NDS03_REG_CMD_VAL, NDS03_CMD_ENA_DISABLE);
    if(ret == NDS03_ERROR_NONE)
    {
        NDS03_ReadByte(pNxDevice, NDS03_REG_CALIB_STATE, (uint8_t*)&ret);
        ret = ret & (NDS03_CALIB_ERROR_XTALK_OVERFLOW|NDS03_CALIB_ERROR_XTALK_EXCESSIVE);
    }
    NX_PRINTF("%s End!\r\n", __func__);

    return ret;
}

/**
 * @brief NDS03 Get Xtalk Value
 *        获取标定串扰值
 * @param pNxDevice 
 * @return NDS03_Error 
 */
NDS03_Error NDS03_GetXTalkValue(NDS03_Dev_t *pNxDevice, uint16_t* xtalk_value)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret |= NDS03_ReadHalfWord(pNxDevice, NDS03_REG_XTALK, xtalk_value);

    return ret;
}
