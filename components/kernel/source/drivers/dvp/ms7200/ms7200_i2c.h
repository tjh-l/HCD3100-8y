#ifndef _MS7200_I2C_H
#define _MS7200_I2C_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define MS7200_DEVID_ADDR          (0x56 >> 1)

/* ****************************************************************************** */
// I2C channel number 0 ~ 3
enum
{
    BUS0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,       // external
    BUS_MAX
};


#define ERR_I2C_BUS_DISABLE             0x01
#define ERR_I2C_NO_ADDRESS_ACK          0x02
#define ERR_I2C_NO_SUBADDRESS_ACK   0x04
#define ERR_I2C_NO_DATA_ACK             0x08

extern uint8_t MS7200I2CWrite(uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen, uint8_t *pbData, uint8_t wData);
extern uint8_t MS7200I2CRead(uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen, uint8_t *pbData, uint8_t wData);

#endif
