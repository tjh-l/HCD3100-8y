/******************************************************************************
* @file    mpi.c
* @author
* @version V1.0.0
* @date    11-Nov-2014
* @brief   MacroSilicon Programming Interface.
*
* Copyright (c) 2009-2014, MacroSilicon Technology Co.,Ltd.
******************************************************************************/
#include "ms7200_comm.h"
#include "ms7200_i2c.h"
#include "../dvp_i2c.h"


VOID mculib_chip_reset(VOID)
{
    printf("%s(%d)\n", __func__, __LINE__);
    #if 0
    ST_GPIO_Init(RESET_PORT, RESET_PIN, GPIO_MODE_OUT_OD_HIZ_FAST);
    ST_GPIO_Init(LED_PORT, GPIO_PIN_ALL, GPIO_MODE_OUT_PP_HIGH_SLOW);
    // 20140512
    // chip reset,send low level pulse > 100us
    ST_GPIO_WriteLow(RESET_PORT, RESET_PIN);
    mculib_delay_ms(1);
    ST_GPIO_WriteHigh(RESET_PORT, RESET_PIN);
    //delay for chip stable
    mculib_delay_ms(1);
    #endif
}

BOOL mculib_chip_read_interrupt_pin(VOID)
{
    printf("%s(%d)\n", __func__, __LINE__);
    #if 0
    return ST_GPIO_ReadInputPin(INT_PORT, INT_PIN);
    #else
    //if no use interrupt pin, must return TRUE
    return TRUE;
    #endif
}

extern int usleep(useconds_t us);
VOID mculib_delay_ms(UINT8 u8_ms)
{
    usleep(1000 * u8_ms);
}

VOID mculib_delay_us(UINT8 u8_us)
{
    usleep(u8_us);
}

//beflow APIs is for 16bits I2C slaver address for access ms933x register, must be implemented
UINT8 mculib_i2c_read_16bidx8bval(UINT8 u8_address, UINT16 u16_index)
{
    UINT8 u8_value = 0;
    //if(MS7200I2CRead(u8_address >> 1, &u16_index, 2, &u8_value, 1) < 0)
    //{
    //    return 0;
    //}
    int ret = 0;
    ret = dvp_i2c_write((UINT8 *)&u16_index, 2);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    ret = dvp_i2c_read(&u8_value, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    return u8_value;
}

BOOL mculib_i2c_write_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT8 u8_value)
{
    #if 0
    int ret = 0;
    printf("%s(%d) u16_index=%04x\n", __FUNCTION__, __LINE__, u16_index);
    ret = dvp_i2c_write((UINT8 *)&u16_index, 2);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    printf("%s(%d) u8_value=%02x\n", __FUNCTION__, __LINE__, u8_value);
    ret = dvp_i2c_write(&u8_value, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    #else
    int ret = 0;
    UINT8 *array = malloc(3);
    if(NULL == array)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    //printf("%s(%d) u16_index=%04x\n", __FUNCTION__, __LINE__, u16_index);
    //printf("%s(%d) u8_value=%02x\n", __FUNCTION__, __LINE__, u8_value);

    memcpy(array, (UINT8 *)&u16_index, 2);
    array[2] = u8_value;
    ret = dvp_i2c_write(array, 3);

    free(array);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    #endif
    return TRUE;
    //return (MS7200I2CWrite(u8_address >> 1, &u16_index, 2, &u8_value, 1) < 0) ? FALSE : TRUE;
}

VOID mculib_i2c_burstread_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT16 u16_length, UINT8 *pu8_value)
{
    int ret = 0;
    ret = dvp_i2c_write((UINT8 *)&u16_index, 2);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    ret = dvp_i2c_read(pu8_value, u16_length);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    //MS7200I2CRead(u8_address >> 1, &u16_index, 2, pu8_value, u16_length);
}

VOID mculib_i2c_burstwrite_16bidx8bval(UINT8 u8_address, UINT16 u16_index, UINT16 u16_length, UINT8 *pu8_value)
{
    #if 0
    int ret = 0;
    printf("%s(%d) u16_index=%04x\n", __FUNCTION__, __LINE__, u16_index);
    ret = dvp_i2c_write((UINT8 *)&u16_index, 2);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    if(u16_length < 8)
    {
        int i = 0;
        for(i = 0; i < u16_length; i++)
            printf("%s(%d) pu8_value[%d]=%02x\n", __FUNCTION__, __LINE__, i, pu8_value[i]);
    }
    ret = dvp_i2c_write(pu8_value, u16_length);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    #else
    int ret = 0;
    UINT8 *array = malloc(u16_length + 2);
    if(NULL == array)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }

    //printf("%s(%d) u16_index=%04x\n", __FUNCTION__, __LINE__, u16_index);
    //if(u16_length < 8)
    //{
    //    int i = 0;
    //    for(i = 0; i < u16_length; i++)
    //        printf("%s(%d) pu8_value[%d]=%02x\n", __FUNCTION__, __LINE__, i, pu8_value[i]);
    //}

    memcpy(array, (UINT8 *)&u16_index, 2);
    memcpy(&array[2], pu8_value, u16_length);
    ret = dvp_i2c_write(array, u16_length + 2);

    free(array);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    #endif
    //MS7200I2CWrite(u8_address >> 1, &u16_index, 2, pu8_value, u16_length);
}

//beflow APIs is for 8bits I2C slaver address, for HDMI TX DDC
//if user need use hdmi tx ddc, must be implemented
//20K is for DDC. 100K is for chip register access
VOID mculib_i2c_set_speed(UINT8 u8_i2c_speed)
{
    printf("%s(%d) u8_i2c_speed=%d\n", __func__, __LINE__, u8_i2c_speed);
    #if 0
    switch(u8_i2c_speed)
    {
        case 0: //I2C_SPEED_20K
            g_u8_i2c_delay = 15; //measure is 20.7KHz
            break;
        case 1: //I2C_SPEED_100K
            g_u8_i2c_delay = 2; //measure is 108.7KHz
            break;
        case 2: //I2C_SPEED_400K
            g_u8_i2c_delay = 1; //measure is 150.6KHz
            break;
        case 3: //I2C_SPEED_750K
            g_u8_i2c_delay = 1; //measure is 150.6KHz
            break;
    }
    #endif
}

UINT8 mculib_i2c_read_8bidx8bval(UINT8 u8_address, UINT8 u8_index)
{
    UINT8 u8_value = 0;
    int ret = 0;
    ret = dvp_i2c_write(&u8_index, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    ret = dvp_i2c_read(&u8_value, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    //if(MS7200I2CRead(u8_address >> 1, &u8_index, 1, &u8_value, 1) < 0)
    //{
    //    return 0;
    //}
    return u8_value;
}

BOOL mculib_i2c_write_8bidx8bval(UINT8 u8_address, UINT8 u8_index, UINT8 u8_value)
{
    int ret = 0;
    ret = dvp_i2c_write(&u8_index, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    ret = dvp_i2c_write(&u8_value, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    return TRUE;
    //return (MS7200I2CWrite(u8_address >> 1, &u8_index, 1, &u8_value, 1) < 0) ? FALSE : TRUE;
}

VOID mculib_i2c_burstread_8bidx8bval(UINT8 u8_address, UINT8 u8_index, UINT8 u8_length, UINT8 *pu8_value)
{
    int ret = 0;
    ret = dvp_i2c_write(&u8_index, 1);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    ret = dvp_i2c_read(pu8_value, u8_length);
    if(ret < 0)
    {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }
    //MS7200I2CRead(u8_address >> 1, &u8_index, 1, pu8_value, u8_length);
}

//8-bit index for HDMI EDID block 2-3 read
//for HDMI CTS test only, not necessary for user
BOOL mculib_i2c_write_blank(UINT8 u8_address, UINT8 u8_index)
{
    printf("%s(%d) u8_address=0x%02x\n", __func__, __LINE__, u8_address);
    #if 0
    BOOL result = FALSE;

    _i2c_start();

    if(!_i2c_write_byte(u8_address))
    { goto STOP; }

    if(!_i2c_write_byte(u8_index))
    { goto STOP; }

    result = TRUE;

STOP:
    ///_i2c_stop();
    return result;
    #endif

    return TRUE;
}

VOID mculib_i2c_burstread_8bidx8bval_ext(UINT8 u8_address, UINT8 u8_index, UINT8 u8_length)
{
    printf("%s(%d) u8_address=0x%02x\n", __func__, __LINE__, u8_address);
    #if 0
    UINT8 i;
    UINT8 read_falg;

    _i2c_start();

    if(!_i2c_write_byte(u8_address))
    { goto STOP; }

    if(!_i2c_write_byte(u8_index))
    { goto STOP; }

    _i2c_start();

    if(!_i2c_write_byte(u8_address | 0x01))
    { goto STOP; }

    for(i = 0; i < (u8_length - 1); i ++)
    {
        read_falg = _i2c_read_byte(FALSE);
        // if(i==0)
        // mculib_uart_log1("read_falg=",read_falg);
    }

    _i2c_read_byte(TRUE);

STOP:
    _i2c_stop();
    #endif
}

//below APIs is for for sdk internal debug, not necessary for user
VOID mculib_uart_log(UINT8 *u8_string)
{
    printf("%s\n", (const char *)u8_string);
}

VOID mculib_uart_log1(UINT8 *u8_string, UINT16 u16_hex)
{
    printf("%s 0x%x\n", (const char *)u8_string, u16_hex);
}

VOID mculib_uart_log2(UINT8 *u8_string, UINT16 u16_dec)
{
    printf("%s %d\n", (const char *)u8_string, u16_dec);
}
