/*
    This file list all the functions for user to implement.
*/

#ifndef __NDS03_PLATFORM__H__
#define __NDS03_PLATFORM__H__
#include "nds03_stdint.h"

/** 
  * @struct NDS03_Platform_t
  * 
  * @brief NDS03平台相关定义 \n
  * 定义i2c地址等，注意必须定义i2c地址
  */
typedef struct{
    /** 用户不可更改以下变量  @{ */
    uint8_t     i2c_dev_addr;   // i2c设备地址
    /** @} */
    uint8_t     xshut_pin;
    /** 用户可根据需要添加变量 */
}NDS03_Platform_t;

/** NDS03使用平台初始化 */
int8_t nds03_platform_init(NDS03_Platform_t *pdev, void *arg);

/** NDS03使用平台释放 */
int8_t nds03_platform_uninit(NDS03_Platform_t *pdev, void *arg);

/** i2c连续读len个字节 */
int8_t nds03_i2c_read_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *read_data, uint16_t len);

/** i2c连续写len个字节 */
int8_t nds03_i2c_write_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *write_data, uint16_t len);

/** 延时wait_ms毫秒 */
int8_t nds03_delay_1ms(NDS03_Platform_t *pDev, uint32_t wait_ms);

/** 延时10*wait_10us微秒 */
int8_t nds03_delay_10us(NDS03_Platform_t *pDev, uint32_t wait_10us);

/** 设置nds03 xshut引脚电平 */
int8_t nds03_set_xshut_pin_level(NDS03_Platform_t *pDev, int8_t level);

#endif

