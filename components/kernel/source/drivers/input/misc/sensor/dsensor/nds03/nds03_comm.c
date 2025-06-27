/**
 * @file nds03_comm.c
 * @author tongsheng.tang
 * @brief NDS03 communication functions
 * @version 2.x.x
 * @date 2024-04
 * 
 * @copyright Copyright (c) 2024, Nephotonics Information Technology (Hefei) Co., Ltd.
 * 
 */

#include "nds03_stdint.h"
#if NDS03_PLATFORM != PLATFORM_LINUX_DRIVER
#include <stdint.h>
#include <stdlib.h>
#else
#include <linux/types.h>
#endif
#include "nds03_dev.h"
#include "nds03_comm.h"
#include "nds03_def.h"
#include "nds03_platform.h"

/**
 * @brief NDS03 Delay 1ms
 * @param   ms 延时时间
 * @return  void  
 */
NDS03_Error NDS03_Delay1ms(NDS03_Dev_t *pNxDevice, uint32_t ms)
{
    return nds03_delay_1ms(&pNxDevice->platform, ms);
} 

/**
 * @brief NDS03 Delay 10us
 * @param   us 延时时间
 * @return  void  
 */
NDS03_Error NDS03_Delay10us(NDS03_Dev_t *pNxDevice, uint32_t us)
{
    return nds03_delay_10us(&pNxDevice->platform, us);
} 

/**
 * @brief NDS03 Set XShut Pin Level
 *        设置xshut引脚的电平
 * @param   pNxDevice   模组设备
 * @param   level       xshut引脚电平，0为低电平，1为高电平
 * @return  void  
 */
NDS03_Error NDS03_SetXShutPinLevel(NDS03_Dev_t *pNxDevice, int8_t level)
{
    return nds03_set_xshut_pin_level(&pNxDevice->platform, level);
}

/**
 * @brief Write n Words to NDS03
 *        对NDS03的寄存器写N个字
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param wdata: 存放寄存器值的指针
 * @param len: 写数据的长度，按字个数计算
 * @return NDS03_Error
*/
NDS03_Error NDS03_WriteNBytes(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *wdata, uint16_t size)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret = nds03_i2c_write_nbytes(&pNxDevice->platform, addr, (uint8_t*)wdata, size);

	return ret;
}


/**
 * @brief Read n Words from NDS03
 *        对NDS03的寄存器读N个字
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param rdata: 存放寄存器值的指针
 * @param len: 读数据的长度，按字个数计算
 * @return NDS03_Error
*/
NDS03_Error NDS03_ReadNBytes(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *rdata, uint16_t size)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret = nds03_i2c_read_nbytes(&pNxDevice->platform, addr, (uint8_t*)rdata, size);

	return ret;
}


/**
 * @brief Write 1 Byte to NDS03
 *        对NDS03的寄存器写1个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param wdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_WriteByte(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t wdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret = nds03_i2c_write_nbytes(&pNxDevice->platform, addr, &wdata, 1);

	return ret;
}

/**
 * @brief Read 1 Byte from NDS03
 *        对NDS03的寄存器读1个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param rdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_ReadByte(NDS03_Dev_t *pNxDevice, uint8_t addr, uint8_t *rdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;

    ret = nds03_i2c_read_nbytes(&pNxDevice->platform, addr, rdata, 1);

	return ret;
}

/**
 * @brief Write 2 Byte to NDS03
 *        对NDS03的寄存器写2个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param wdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_WriteHalfWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint16_t wdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         tmp[2];

    tmp[0] = (wdata>>0) & 0xFF;
    tmp[1] = (wdata>>8) & 0xFF;
    ret = nds03_i2c_write_nbytes(&pNxDevice->platform, addr, tmp, sizeof(tmp));

	return ret;
}


/**
 * @brief Read 2 Byte from NDS03
 *        对NDS03的寄存器读2个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param rdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_ReadHalfWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint16_t *rdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         tmp[2];

    ret = nds03_i2c_read_nbytes(&pNxDevice->platform, addr, tmp, sizeof(tmp));
    if(rdata != NULL)
    {
        *rdata = (uint16_t)tmp[0] | ((uint16_t)tmp[1]<<8);
    }

	return ret;
}

/**
 * @brief Write 2 Byte to NDS03
 *        对NDS03的寄存器写2个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param wdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_WriteWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint32_t wdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         tmp[4];

    tmp[0] = (wdata>>0) & 0xFF;
    tmp[1] = (wdata>>8) & 0xFF;
    tmp[2] = (wdata>>16) & 0xFF;
    tmp[3] = (wdata>>24) & 0xFF;
    ret = nds03_i2c_write_nbytes(&pNxDevice->platform, addr, tmp, sizeof(tmp));

	return ret;
}


/**
 * @brief Read 2 Byte from NDS03
 *        对NDS03的寄存器读2个字节
 * @param pNxDevice: NDS03模组设备信息结构体指针
 * @param addr: 寄存器地址
 * @param rdata: 寄存器的值
 * @return NDS03_Error
*/
NDS03_Error NDS03_ReadWord(NDS03_Dev_t *pNxDevice, uint8_t addr, uint32_t *rdata)
{
    NDS03_Error     ret = NDS03_ERROR_NONE;
    uint8_t         tmp[4];

    ret = nds03_i2c_read_nbytes(&pNxDevice->platform, addr, tmp, sizeof(tmp));
    if(rdata != NULL)
    {
        *rdata = (uint32_t)tmp[0] | ((uint32_t)tmp[1]<<8) | ((uint32_t)tmp[2]<<16) | ((uint32_t)tmp[3]<<24);
    }

	return ret;
}

