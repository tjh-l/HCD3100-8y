#ifndef PM5310_H
#define PM5310_H

/*battery addr */  
typedef enum {
    REG_BATTERY_LEVEL    =0x74,  	   //Detect battery level
    REG_CHARGING_STATE   =0x70,  	  //Detect charging state
    REG_FULL_CHARGE_STATE=0x71,  //Detect full charge state
    REG_LIGHT_LOAD_MODE  =0X02,	//Light load mode switch
}pm5310_reg;

/*battery level */  
typedef enum {
    PM5310_BATTERY_LEVEL_100 =0xf0,  //capacity >= 75%
    PM5310_BATTERY_LEVEL_75  =0x70,  //50% < capacity <= 75%
    PM5310_BATTERY_LEVEL_50  =0x30,  //25% < capacity <= 50%
    PM5310_BATTERY_LEVEL_25  =0x10,  //3% < capacity <= 25%
    PM5310_BATTERY_LEVEL_3   =0x00,  //capacity <= 3%
    PM5310_IN_CHARGING_STATE =0x08,   //charging state
    PM5310_NOT_FULL_CHARGED_STATE=0x00, //not fully charged state
}pm5310_state;

int pm5310_i2c_probe(void);  //read pm5310 device state
int i2c_write_pm5310_state(unsigned char buf_write_addr, unsigned char buf_write_bit, int write_mode);
int i2c_read_pm5310_state(unsigned char buf_read_addr, unsigned char *buf_read, unsigned char buf_read_bit);

#endif