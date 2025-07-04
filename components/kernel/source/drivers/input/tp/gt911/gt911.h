#ifndef _GOODIX_GT9XX_H_
#define _GOODIX_GT9XX_H_

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <kernel/lib/fdt_api.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>

//formyself
//
#define CONFIG_OF

//***************************PART1:ON/OFF define*******************************
#define GTP_CUSTOM_CFG        0
#define GTP_CHANGE_X2Y        0
#define GTP_DRIVER_SEND_CFG   0
#define GTP_HAVE_TOUCH_KEY    0
#define GTP_POWER_CTRL_SLEEP  0
#define GTP_ICS_SLOT_REPORT   1

#define GTP_AUTO_UPDATE       0    // auto update fw by .bin file as default
#define GTP_HEADER_FW_UPDATE  0    // auto update fw by gtp_default_FW in gt9xx_firmware.h, function together with GTP_AUTO_UPDATE
#define GTP_AUTO_UPDATE_CFG   0    // auto update config by .cfg file, function together with GTP_AUTO_UPDATE

#define GTP_COMPATIBLE_MODE   0    // compatible with GT9XXF

#define GTP_CREATE_WR_NODE    0
#define GTP_ESD_PROTECT       0    // esd protection with a cycle of 2 seconds

#define GTP_WITH_PEN          0
#define GTP_PEN_HAVE_BUTTON   0    // active pen has buttons, function together with GTP_WITH_PEN

#define GTP_GESTURE_WAKEUP    0    // gesture wakeup
#define HOTKNOT_ENABLE        1    // hotknot module
#define HOTKNOT_BLOCK_RW      0    // block rw operation in hotknot 
#define FLASHLESS_FLASH_WORKROUND  0

#define GTP_DEBUG_ON          0
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0
 
#if HOTKNOT_BLOCK_RW

extern u8 wait_hotknot_state;
static u8 got_hotknot_state;
extern u8 got_hotknot_extra_state;
extern u8 hotknot_paired_flag;
extern wait_queue_head_t bp_waiter;
#endif

#if HOTKNOT_ENABLE
extern u8 hotknot_enabled;
#endif

#if GTP_COMPATIBLE_MODE
typedef enum {
	CHIP_TYPE_GT9 = 0,
	CHIP_TYPE_GT9F = 1,
} CHIP_TYPE_T;
#endif

struct goodix_ts_data {
	int np;
	spinlock_t irq_lock;
	const char *i2c_devpath;
	unsigned int addr;
	unsigned int irq;
	unsigned int reset_pin;
	struct input_dev *input_dev;
	//    struct hrtimer timer;
	struct work_s work;
	s32 irq_is_disable;
	s32 use_irq;
	u16 abs_x_max;
	u16 abs_y_max;
	u8 max_touch_num;
	u8 int_trigger_type;
	u8 enter_update;
	u8 gtp_is_suspend;
	u8 gtp_rawdiff_mode;
	int gtp_cfg_len;
	u8 fw_error;
	u8 pnl_init_error;
#if defined(CONFIG_FB)
	struct notifier_block notifier;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif

#if GTP_WITH_PEN
	struct input_dev *pen_dev;
#endif

#if GTP_ESD_PROTECT
	spinlock_t esd_lock;
	u8 esd_running;
	s32 clk_tick_cnt;
#endif

#if GTP_COMPATIBLE_MODE
	u16 bak_ref_len;
	s32 ref_chk_fs_times;
	s32 clk_chk_fs_times;
	CHIP_TYPE_T chip_type;
	u8 rqst_processing;
	u8 is_950;
#endif
	struct work_struct *worker;
	struct delayed_work ddeel_data_work;
};

extern u16 show_len;
extern u16 total_len;
extern int gtp_rst_gpio;
extern int gtp_int_gpio;
extern u8 gtp_resetting;

//*************************** PART2:TODO define **********************************
// STEP_1(REQUIRED): Define Configuration Information Group(s)
// Sensor_ID Map:
/* sensor_opt1 sensor_opt2 Sensor_ID
    GND         GND         0 
    VDDIO       GND         1 
    NC          GND         2 
    GND         NC/300K     3 
    VDDIO       NC/300K     4 
    NC          NC/300K     5 
*/
// TODO: define your own default or for Sensor_ID == 0 config here. 
// The predefined one is just a sample config, which is not suitable for your tp in most cases.
#define CTP_CFG_GROUP0 {\
    0x48,0xD0,0x02,0x00,0x05,0x05,0x34,0x00,0x01,0x8C,\
    0x1E,0x0C,0x50,0x3C,0x03,0x07,0x01,0x01,0xFF,0xFF,\
    0x00,0x00,0x00,0x18,0x1A,0x1E,0x14,0x8B,0x2B,0x0C,\
    0x50,0x52,0xD6,0x09,0x03,0x00,0x00,0x9C,0x32,0x1D,\
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
    0xF4,0x4A,0x64,0x9E,0xE5,0x01,0x14,0x00,0x00,0x04,\
    0x74,0x4C,0x00,0x70,0x50,0x00,0x69,0x55,0x00,0x63,\
    0x5B,0x00,0x5E,0x61,0x00,0x5E,0x00,0x00,0x00,0x00,\
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
    0x00,0x01,0x1B,0x14,0x0D,0x14,0x03,0x0F,0x0A,0x0F,\
    0x0F,0x03,0x07,0x10,0x42,0x86,0x0E,0x05,0x00,0x00,\
    0x00,0x50,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,\
    0x12,0x14,0x16,0x18,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
    0xFF,0xFF,0x00,0x01,0x02,0x04,0x06,0x07,0x08,0x09,\
    0x0A,0x0C,0x0E,0x1D,0x1E,0x1F,0x20,0x22,0x24,0x25,\
    0x26,0x28,0x29,0x2A,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
    0xFF,0xFF,0xFF,0xFF,0x5A,0x01,\
    }

// TODO: define your config for Sensor_ID == 1 here, if needed
#define CTP_CFG_GROUP1 {\
    }

// TODO: define your config for Sensor_ID == 1 here, if needed
#define CTP_CFG_GROUP2 {\
    }

// TODO: define your config for Sensor_ID == 2 here, if needed
#define CTP_CFG_GROUP3 {\
    }

// TODO: define your config for Sensor_ID == 3 here, if needed
#define CTP_CFG_GROUP4 {\
}
// TODO: define your config for Sensor_ID == 4 here, if needed
#define CTP_CFG_GROUP5 {\
    }
#define GTP_MAX_CFG_GROUP		6

// STEP_2(REQUIRED): Customize your I/O ports & I/O operations
#define GTP_RST_PORT 	16//S5PV210_GPJ3(6)
#define GTP_INT_PORT    17//S5PV210_GPH1(3)
#if 0
#define GTP_GPIO_AS_INPUT(pin)          do{\
                                            gpio_direction_input(pin);\
                                        }while(0)
#endif
#define GTP_GPIO_AS_INPUT(pin)          do{\
                                            gpio_configure(pin,GPIO_DIR_INPUT);\
                                        }while(0)
#define GTP_GPIO_AS_INT(pin)            do{\
                                            GTP_GPIO_AS_INPUT(pin);\
                                        }while(0)
#define GTP_GPIO_GET_VALUE(pin)         gpio_get_value(pin)
//#define GTP_GPIO_OUTPUT(pin,level)      gpio_direction_output(pin,level)
#define GTP_GPIO_OUTPUT(pin,level)      do{\
                                            gpio_configure(pin,GPIO_DIR_OUTPUT);\
                                            gpio_set_output(pin,level);\
                                        }while(0)
                                        
//#define GTP_GPIO_REQUEST(pin, label)    gpio_request(pin, label)
#define GTP_GPIO_REQUEST(pin, label)    
//#define GTP_GPIO_FREE(pin)              gpio_free(pin)
#define GTP_GPIO_FREE(pin)              
#define GTP_IRQ_TAB                     {IRQ_TYPE_EDGE_RISING, IRQ_TYPE_EDGE_FALLING, IRQ_TYPE_LEVEL_LOW, IRQ_TYPE_LEVEL_HIGH}

// STEP_3(optional): Specify your special config info if needed
#if GTP_CUSTOM_CFG
  #define GTP_MAX_HEIGHT   800
  #define GTP_MAX_WIDTH    480
  #define GTP_INT_TRIGGER  0            // 0: Rising 1: Falling
#else
  #define GTP_MAX_HEIGHT   4096
  #define GTP_MAX_WIDTH    4096
  #define GTP_INT_TRIGGER  1
#endif
#define GTP_MAX_TOUCH         5

// STEP_4(optional): If keys are available and reported as keys, config your key info here                             
#if GTP_HAVE_TOUCH_KEY
    #define GTP_KEY_TAB  {KEY_MENU, KEY_HOME, KEY_BACK}
#endif

//***************************PART3:OTHER define*********************************
#define GTP_DRIVER_VERSION          "V2.4<2014/11/28>"
#define GTP_I2C_NAME                "Goodix-TS"
#define GT91XX_CONFIG_PROC_FILE     "gt9xx_config"
#define GTP_POLL_TIME         10    
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_MIN_LENGTH 186
#define GTP_CONFIG_MAX_LENGTH 240
#define FAIL                  0
#define SUCCESS               1
#define SWITCH_OFF            0
#define SWITCH_ON             1

//******************** For GT9XXF Start **********************//
#define GTP_REG_BAK_REF                 0x99D0
#define GTP_REG_MAIN_CLK                0x8020
#define GTP_REG_CHIP_TYPE               0x8000
#define GTP_REG_HAVE_KEY                0x804E
#define GTP_REG_MATRIX_DRVNUM           0x8069     
#define GTP_REG_MATRIX_SENNUM           0x806A

#define GTP_FL_FW_BURN              0x00
#define GTP_FL_ESD_RECOVERY         0x01
#define GTP_FL_READ_REPAIR          0x02

#define GTP_BAK_REF_SEND                0
#define GTP_BAK_REF_STORE               1
#define CFG_LOC_DRVA_NUM                29
#define CFG_LOC_DRVB_NUM                30
#define CFG_LOC_SENS_NUM                31

#define GTP_CHK_FW_MAX                  40
#define GTP_CHK_FS_MNT_MAX              300
#define GTP_BAK_REF_PATH                "/data/gtp_ref.bin"
#define GTP_MAIN_CLK_PATH               "/data/gtp_clk.bin"
#define GTP_RQST_CONFIG                 0x01
#define GTP_RQST_BAK_REF                0x02
#define GTP_RQST_RESET                  0x03
#define GTP_RQST_MAIN_CLOCK             0x04
#define GTP_RQST_HOTKNOT_CODE           0x20
#define GTP_RQST_RESPONDED              0x00
#define GTP_RQST_IDLE                   0xFF

#define HN_DEVICE_PAIRED                0x80
#define HN_MASTER_DEPARTED              0x40
#define HN_SLAVE_DEPARTED               0x20
#define HN_MASTER_SEND                  0x10
#define HN_SLAVE_RECEIVED               0x08


//******************** For GT9XXF End **********************//
// Registers define
#define GTP_READ_COOR_ADDR    0x814E
#define GTP_REG_SLEEP         0x8040
#define GTP_REG_SENSOR_ID     0x814A
#define GTP_REG_CONFIG_DATA   0x8047
#define GTP_REG_VERSION       0x8140
#define GTP_REG_HN_STATE      0xAB10

#define RESOLUTION_LOC        3
#define TRIGGER_LOC           8

#define CFG_GROUP_LEN(p_cfg_grp)  (sizeof(p_cfg_grp) / sizeof(p_cfg_grp[0]))
// Log define
#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->> "fmt"\n",##arg)
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->> "fmt"\n",##arg)
#define GTP_DEBUG(fmt,arg...)          do{\
                                         if(GTP_DEBUG_ON)\
                                         printk("<<-GTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)
#define GTP_DEBUG_ARRAY(array, num)    do{\
                                         s32 i;\
                                         u8* a = array;\
                                         if(GTP_DEBUG_ARRAY_ON)\
                                         {\
                                            printk("<<-GTP-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printk("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printk("\n");\
                                                }\
                                            }\
                                            printk("\n");\
                                        }\
                                       }while(0)
#define GTP_DEBUG_FUNC()               do{\
                                         if(GTP_DEBUG_FUNC_ON)\
                                         printk("<<-GTP-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)
#define GTP_SWAP(x, y)                 do{\
                                         typeof(x) z = x;\
                                         x = y;\
                                         y = z;\
                                       }while (0)

//*****************************End of Part III********************************
extern int gtp_parse_dt_cfg(struct goodix_ts_data *ts, u8 *cfg, int *cfg_len, u8 sid);
#endif /* _GOODIX_GT9XX_H_ */
