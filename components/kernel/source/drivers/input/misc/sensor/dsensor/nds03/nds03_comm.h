/**
 * @file nds03_comm.h
 * @author tongsheng.tang
 * @brief NDS03 communication functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */
#ifndef __NDS03_COMM_H__
#define __NDS03_COMM_H__

#include "nds03_def.h"

/** @defgroup NDS03_Communication_Group NDS03 Communication Funtions
 *  @brief NDS03 Communication Funtions
 *  @{
 */

/** 延时时间（ms） */
NDS03_Error NDS03_Delay1ms(NDS03_Dev_t *pNxDevice, uint32_t ms);

/** 延时时间（10us） */
NDS03_Error NDS03_Delay10us(NDS03_Dev_t *pNxDevice, uint32_t us);

/** 设置xshut引脚的电平 */
NDS03_Error NDS03_SetXShutPinLevel(NDS03_Dev_t *pNxDevice, int8_t level);

/** 对NDS03寄存器写N个字节 */
NDS03_Error NDS03_WriteNBytes(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *wdata, uint16_t size);
/** 对NDS03寄存器读N个字节 */
NDS03_Error NDS03_ReadNBytes(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *rdata, uint16_t size);
/** 对NDS03寄存器写1个字节 */
NDS03_Error NDS03_WriteByte(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t wdata);
/** 对NDS03寄存器写1个字节 */
NDS03_Error NDS03_ReadByte(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *rdata);
/** 对NDS03寄存器写1个字 */
NDS03_Error NDS03_WriteHalfWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint16_t wdata);
/** 对NDS03寄存器读1个字 */
NDS03_Error NDS03_ReadHalfWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint16_t *rdata);
/** 对NDS03寄存器写1个字 */
NDS03_Error NDS03_WriteWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint32_t wdata);
/** 对NDS03寄存器读1个字 */
NDS03_Error NDS03_ReadWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint32_t *rdata);


/** @} NDS03_Communication_Group */

#endif

