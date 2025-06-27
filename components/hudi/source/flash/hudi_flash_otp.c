/**
* @file
* @brief                hudi flash otp interface
* @par Copyright(c):    Hichip Semiconductor (c) 2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#ifndef __HCRTOS__
#include <termios.h>
#include <sys/mman.h>
#endif

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_flash.h>
#include "hudi_flash_inter.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

//----------------------------- Flash OTP read--------------------------------
typedef volatile struct sf_reg
{

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint32_t dma_stop_cmd:          1;
            /**
            bitpos: [[1]]
             */
            uint32_t dma_stop_int_en:           1;
            uint32_t reserved2:         7;
            /**
            bitpos: [[10:9]]
             */
            uint32_t spi_mode:          2;
            uint32_t reserved11:            5;
            /**
            bitpos: [[16]]
             */
            uint32_t send_dummy:            1;
            uint32_t reserved17:            7;
            /**
            bitpos: [[24]]
             */
            uint32_t pio_req:           1;
            /**
            bitpos: [[25]]
             */
            uint32_t pio_req_int_en:            1;
            uint32_t reserved26:            6;
        };
        uint32_t val;
    } cpu_dma_ctrl; /* REG_SF_BASE + 0x0 */

    union
    {
        struct
        {
            /**
            bitpos: [[5:0]]
             */
            uint32_t clk_div_ext:           6;
            uint32_t reserved6:         26;
        };
        uint32_t val;
    } clk_div_ext;  /* REG_SF_BASE + 0x4 */

    uint32_t reserved_8; /* REG_SF_BASE + 0x8 */
    uint32_t reserved_c; /* REG_SF_BASE + 0xc */
    uint32_t reserved_10; /* REG_SF_BASE + 0x10 */
    uint32_t reserved_14; /* REG_SF_BASE + 0x14 */
    uint32_t reserved_18; /* REG_SF_BASE + 0x18 */
    uint32_t reserved_1c; /* REG_SF_BASE + 0x1c */
    uint32_t reserved_20; /* REG_SF_BASE + 0x20 */
    uint32_t reserved_24; /* REG_SF_BASE + 0x24 */
    uint32_t reserved_28; /* REG_SF_BASE + 0x28 */
    uint32_t reserved_2c; /* REG_SF_BASE + 0x2c */
    uint32_t reserved_30; /* REG_SF_BASE + 0x30 */
    uint32_t reserved_34; /* REG_SF_BASE + 0x34 */
    uint32_t reserved_38; /* REG_SF_BASE + 0x38 */
    uint32_t reserved_3c; /* REG_SF_BASE + 0x3c */
    uint32_t reserved_40; /* REG_SF_BASE + 0x40 */
    uint32_t reserved_44; /* REG_SF_BASE + 0x44 */
    uint32_t reserved_48; /* REG_SF_BASE + 0x48 */
    uint32_t reserved_4c; /* REG_SF_BASE + 0x4c */
    uint32_t reserved_50; /* REG_SF_BASE + 0x50 */
    uint32_t reserved_54; /* REG_SF_BASE + 0x54 */

    union
    {
        struct
        {
            /**
            bitpos: [[29:0]]
             */
            uint32_t dma_mem_addr:          30;
            uint32_t reserved30:            2;
        };
        uint32_t val;
    } dma_mem_addr; /* REG_SF_BASE + 0x58 */

    union
    {
        struct
        {
            /**
            bitpos: [[25:0]]
             */
            uint32_t dma_flash_addr:            26;
            uint32_t reserved26:            6;
        };
        uint32_t val;
    } dma_flash_addr;   /* REG_SF_BASE + 0x5c */

    union
    {
        struct
        {
            /**
            bitpos: [[23:0]]
             */
            uint32_t dma_len:           24;
            uint32_t reserved24:            8;
        };
        uint32_t val;
    } dma_len;  /* REG_SF_BASE + 0x60 */

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint32_t cs_sel:            1;
            uint32_t reserved1:         4;
            /**
            bitpos: [[5]]
             */
            uint32_t dma_start:         1;
            /**
            bitpos: [[6]]
             */
            uint32_t dma_inc_valid:         1;
            /**
            bitpos: [[7]]
             */
            uint32_t dma_dir:           1;
            uint32_t reserved8:         9;
            /**
            bitpos: [[17]]
             */
            uint32_t arbit_mode:            1;
            /**
            bitpos: [[18]]
             */
            uint32_t access_req:            1;
            /**
            bitpos: [[19]]
             */
            uint32_t access_mode:           1;
            /**
            bitpos: [[20]]
             */
            uint32_t dma_int_en:            1;
            /**
            bitpos: [[21]]
             */
            uint32_t arbit_int_en:          1;
            /**
            bitpos: [[22]]
             */
            uint32_t byte_transfer_en:          1;
            uint32_t reserved23:            1;
            /**
            bitpos: [[24]]
             */
            uint32_t auto_pause_en:         1;
            /**
            bitpos: [[27:25]]
             */
            uint32_t auto_pause_thr:            3;
            /**
            bitpos: [[28]]
             */
            uint32_t dummy_for_addr:            1;
            /**
            bitpos: [[29]]
             */
            uint32_t dummy_inc_en:          1;
            /**
            bitpos: [[30]]
             */
            uint32_t dummy_byte_exchange:           1;
            /**
            bitpos: [[31]]
             */
            uint32_t pre_read_en:           1;
        };
        uint32_t val;
    } dma_ctrl; /* REG_SF_BASE + 0x64 */

    uint32_t reserved_68; /* REG_SF_BASE + 0x68 */
    uint32_t reserved_6c; /* REG_SF_BASE + 0x6c */
    uint32_t reserved_70; /* REG_SF_BASE + 0x70 */
    uint32_t reserved_74; /* REG_SF_BASE + 0x74 */
    uint32_t reserved_78; /* REG_SF_BASE + 0x78 */
    uint32_t reserved_7c; /* REG_SF_BASE + 0x7c */
    uint32_t reserved_80; /* REG_SF_BASE + 0x80 */
    uint32_t reserved_84; /* REG_SF_BASE + 0x84 */
    uint32_t reserved_88; /* REG_SF_BASE + 0x88 */
    uint32_t reserved_8c; /* REG_SF_BASE + 0x8c */
    uint32_t reserved_90; /* REG_SF_BASE + 0x90 */
    uint32_t reserved_94; /* REG_SF_BASE + 0x94 */

    union
    {
        struct
        {
            /**
            bitpos: [[7:0]]
             */
            uint8_t instruct:           8;
        };
        uint8_t val;
    } conf0;    /* REG_SF_BASE + 0x98 */

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint8_t data_hit:           1;
            /**
            bitpos: [[1]]
             */
            uint8_t dummy_hit:          1;
            /**
            bitpos: [[2]]
             */
            uint8_t addr_hit:           1;
            /**
            bitpos: [[3]]
             */
            uint8_t code_hit:           1;
            /**
            bitpos: [[5:4]]
             */
            uint8_t dummy_num:          2;
            /**
            bitpos: [[6]]
             */
            uint8_t continue_read:          1;
            /**
            bitpos: [[7]]
             */
            uint8_t continue_write:         1;
        };
        uint8_t val;
    } conf1;    /* REG_SF_BASE + 0x99 */

    union
    {
        struct
        {
            /**
            bitpos: [[2:0]]
             */
            uint8_t mode:           3;
            /**
            bitpos: [[4:3]]
             */
            uint8_t addr_bytes:         2;
            /**
            bitpos: [[5]]
             */
            uint8_t continue_count_en:          1;
            /**
            bitpos: [[6]]
             */
            uint8_t rx_ready:           1;
            /**
            bitpos: [[7]]
             */
            uint8_t odd_div_setting:            1;
        };
        uint8_t val;
    } conf2;    /* REG_SF_BASE + 0x9a */

    union
    {
        struct
        {
            /**
            bitpos: [[3:0]]
             */
            uint8_t clk_div:            4;
            /**
            bitpos: [[4]]
             */
            uint8_t wj_ctrl:            1;
            /**
            bitpos: [[5]]
             */
            uint8_t holdj_en:           1;
            /**
            bitpos: [[7:6]]
             */
            uint8_t size:           2;
        };
        uint8_t val;
    } conf3;    /* REG_SF_BASE + 0x9b */

    union
    {
        struct
        {
            /**
            bitpos: [[31:0]]
             */
            uint32_t dummy_data:            32;
        };
        uint32_t val;
    } conf4;    /* REG_SF_BASE + 0x9c */

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint32_t dma_int_st:            1;
            uint32_t reserved1:         7;
            /**
            bitpos: [[8]]
             */
            uint32_t arbit_int_st:          1;
            uint32_t reserved9:         9;
            /**
            bitpos: [[18]]
             */
            uint32_t dma_stop_st:           1;
            uint32_t reserved19:            5;
            /**
            bitpos: [[24]]
             */
            uint32_t pio_req_int_st:            1;
            /**
            bitpos: [[25]]
             */
            uint32_t dma_len_err_st:            1;
            uint32_t reserved26:            6;
        };
        uint32_t val;
    } int_st;   /* REG_SF_BASE + 0xa0 */

    uint32_t reserved_a4; /* REG_SF_BASE + 0xa4 */

    union
    {
        struct
        {
            /**
            bitpos: [[7:0]]
             */
            uint32_t latency:           8;
            /**
            bitpos: [[15:8]]
             */
            uint32_t hi_prio_cnt:           8;
            uint32_t reserved16:            16;
        };
        uint32_t val;
    } access_conf;  /* REG_SF_BASE + 0xa8 */

    uint32_t reserved_ac; /* REG_SF_BASE + 0xac */
    uint32_t reserved_b0; /* REG_SF_BASE + 0xb0 */
    uint32_t reserved_b4; /* REG_SF_BASE + 0xb4 */

    union
    {
        struct
        {
            /**
            bitpos: [[7:0]]
             */
            uint32_t addr_byte_prog:            8;
            /**
            bitpos: [[8]]
             */
            uint32_t addr_byte_prog_en:         1;
            uint32_t reserved9:         23;
        };
        uint32_t val;
    } addr_conf;    /* REG_SF_BASE + 0xb8 */
    uint32_t sqi_count; /* REG_SF_BASE + 0xbc */

    union
    {
        struct
        {
            uint32_t reserved0:         2;
            /**
            bitpos: [[23:2]]
             */
            uint32_t addr:          22;
            uint32_t reserved24:            7;
            /**
            bitpos: [[31]]
             */
            uint32_t enable:            1;
        };
        uint32_t val;
    } write_protect_start;  /* REG_SF_BASE + 0xc0 */

    union
    {
        struct
        {
            uint32_t reserved0:         2;
            /**
            bitpos: [[23:2]]
             */
            uint32_t addr:          22;
            uint32_t reserved24:            8;
        };
        uint32_t val;
    } write_protect_end;    /* REG_SF_BASE + 0xc4 */

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint32_t cs_setup_en:           1;
            /**
            bitpos: [[7:1]]
             */
            uint32_t cs_setup:          7;
            /**
            bitpos: [[8]]
             */
            uint32_t cs_hold_en:            1;
            /**
            bitpos: [[15:9]]
             */
            uint32_t cs_hold:           7;
            /**
            bitpos: [[16]]
             */
            uint32_t cs_de_assert_en:           1;
            /**
            bitpos: [[23:17]]
             */
            uint32_t cs_de_assert:          7;
            /**
            bitpos: [[24]]
             */
            uint32_t cs_program_en:         1;
            /**
            bitpos: [[26:25]]
             */
            uint32_t cs_program:            2;
            uint32_t reserved27:            5;
        };
        uint32_t val;
    } timing_ctrl;  /* REG_SF_BASE + 0xc8 */

    uint32_t reserved_cc; /* REG_SF_BASE + 0xcc */

    union
    {
        struct
        {
            /**
            bitpos: [[0]]
             */
            uint32_t crc_valid:         1;
            uint32_t reserved1:         7;
            /**
            bitpos: [[8]]
             */
            uint32_t crc_clear:         1;
            uint32_t reserved9:         23;
        };
        uint32_t val;
    } crc_ctrl; /* REG_SF_BASE + 0xd0 */
    uint32_t crc_init_value; /* REG_SF_BASE + 0xd4 */
    uint32_t crc_result; /* REG_SF_BASE + 0xd8 */

} sf_reg_t;

#define PQ_REG_BASE     (PQ_REG_BASE_ADDR & ~(0x7<<29)) //(0x18836000) 
#define MAP_SIZE 0x400000

static uint8_t reg_98, reg_99, reg_9a, reg_9b, reg_9c, reg_c8;
static inline void _hudi_flash_otp_save_st(sf_reg_t *reg)
{
    reg_98 = reg->conf0.val;
    reg_99 = reg->conf1.val;
    reg_9a = reg->conf2.val;
    reg_9b = reg->conf3.val;
    reg_9c = reg->conf4.val;
    reg_c8 = reg->timing_ctrl.val;
}

static inline void _hudi_flash_otp_restore_st(sf_reg_t *reg)
{
    reg->conf0.val = reg_98;
    reg->conf1.val = reg_99;
    reg->conf2.val = reg_9a;
    reg->conf3.val = reg_9b;
    reg->conf4.val = reg_9c;
    reg->timing_ctrl.val = reg_c8;
}

static inline void _hudi_flash_otp_init(sf_reg_t *reg, uint8_t *addr)
{
    reg->conf0.val = 0x03;
    reg->conf1.val = 0x0d;
    reg->conf2.val = 0x00;
    reg->conf3.val = 0x03;
    reg->conf4.val = 0x00;
    reg->timing_ctrl.val = 0x00;
}

static void _hudi_flash_nor_otp_write_transfer_one(sf_reg_t *reg, uint8_t *sf_addr,
                                                   uint8_t cmd, uint8_t dummy, long long addr,
                                                   uint8_t *wdata, uint32_t len)
{
    uint32_t i;
    uint8_t data = 0;

    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (wdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;

    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;

    if (addr >= 0)
        reg->conf1.addr_hit = 0x01;
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }

    if (len > 1)
        reg->conf1.continue_write = 0x01;
    else
        reg->conf1.continue_write = 0x00;

#if 0
    for (i = 0; i < len - 1; i++)
    {
        sf_addr[addr + i] = wdata[i];
    }
    reg->conf1.continue_write = 0x00;
    sf_addr[addr + i] = wdata[i];
#else
    for (i = 0; i < len; i++)
    {
        sf_addr[addr + i] = wdata[i];
    }
#endif

    if (len == 0)
    {
        sf_addr[addr] = 0;
        usleep(10000);
    }

    reg->conf1.continue_write = 0x00;
    return;
}

static void _hudi_flash_otp_write_transfer_one2(sf_reg_t *reg, uint8_t *sf_addr,
                                                uint8_t cmd, uint8_t dummy, long long addr,
                                                uint8_t *wdata, uint32_t len)
{
    uint32_t i;
    uint8_t data = 0;
    uint8_t tmp = 0;

    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (wdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;

    tmp = reg->conf3.val;
    reg->conf3.val = 0xc3;

    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;

    if (addr >= 0)
        reg->conf1.addr_hit = 0x01;
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }

    if (len > 1)
        reg->conf1.continue_write = 0x01;
    else
        reg->conf1.continue_write = 0x00;

#if 0
    for (i = 0; i < len - 1; i++)
    {
        sf_addr[addr + i] = wdata[i];
    }
    reg->conf1.continue_write = 0x00;
    sf_addr[addr + i] = wdata[i];
#else
    for (i = 0; i < len; i++)
    {
        sf_addr[addr + i] = wdata[i];
    }
#endif

    if (len == 0)
    {
        sf_addr[addr] = 0;
        usleep(10000);
    }

    reg->conf3.val = tmp;
    reg->conf1.continue_write = 0x00;
    return;
}

static inline void _hudi_flash_nor_otp_read_transfer_one(sf_reg_t *reg, uint8_t *sf_addr,
                                                         uint8_t cmd, uint8_t dummy, long long addr,
                                                         uint8_t *rdata, uint32_t len)
{
    uint32_t i;
    uint8_t data = 0;

    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (rdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;
    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;

    if (addr >= 0)
        reg->conf1.addr_hit = 0x01;
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }

    if (len > 1)
        reg->conf1.continue_read = 0x01;
    else
        reg->conf1.continue_read = 0x00;

    for (i = 0; i < len; i++)
    {
        rdata[i] = sf_addr[addr + i];
    }

    if (len == 0)
    {
        sf_addr[0] = 0;
        usleep(10000);
    }

    reg->conf1.continue_read = 0x00;
    return;
}

static inline void _hudi_flash_nor_otp_read_transfer_one2(sf_reg_t *reg, uint8_t *sf_addr,
                                                          uint8_t cmd, uint8_t dummy, long long addr,
                                                          uint8_t *rdata, uint32_t len)
{
    uint32_t i;
    uint8_t data = 0;
    uint8_t tmp = 0;
    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (rdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;

    tmp = reg->conf3.val;
    reg->conf3.val = 0xc3;

    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;


    if (addr >= 0)
        reg->conf1.addr_hit = 0x01;
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }


    if (len > 1)
        reg->conf1.continue_read = 0x01;
    else
        reg->conf1.continue_read = 0x00;


    for (i = 0; i < len; i++)
    {
        rdata[i] = sf_addr[addr + i];
    }


    if (len == 0)
    {
        rdata[i] = sf_addr[0];
        usleep(10000);
    }

    reg->conf3.val = tmp;
    reg->conf1.continue_read = 0x00;

    return;
}

static void _hudi_flash_nor_otp_wait_read_empty(sf_reg_t *reg, uint8_t *addr, uint32_t timeout)
{
    uint8_t temp;
    uint32_t count = 0;

    _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x05, 0, -1, &temp, 1);
    while ((temp >> 0x0) & 0x1)
    {
        _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x05, 0, -1, &temp, 1);
        usleep(1000);
        count++;
        if (count == timeout)
        {
            return;
        }
    }
    return;
}

static inline void _hudi_flash_nor_otp_read_id(sf_reg_t *reg, uint8_t *addr, uint8_t *id)
{
    uint8_t temp;
    _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
    _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x9f, 0, -1, id, 3);
    _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
    _hudi_flash_nor_otp_wait_read_empty(reg, addr, 1000);
}

static inline int _hudi_flash_nor_otp_read_uid(sf_reg_t *reg, uint8_t *addr,
                                               uint8_t *uid, unsigned int *len)
{
    uint32_t id;
    uint8_t i;
    uint8_t buf[16] = {0};
    uint8_t temp;

    _hudi_flash_nor_otp_read_id(reg, addr, buf);
    id = buf[2] | (buf[1] << 8) | (buf[0] << 16);

    //printf("id: %x\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0x00FFFFFF)
    {
        case 0x1c7117:  //ME25L64A
        case 0x1c7119:  //EN25QX128A
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x5a, 1, 0x01e0, uid, 16);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            break;
        case 0x1c7118:  //EN25QX128A
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x5a, 1, 0x000080, uid, 16);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            *len = 12;
#if 0
            printf("UID:");
            for (i = 0; i < 16; i++)
            {
                printf("0x%.2x ", buf[i]);
            }
            printf("\n");
#endif
            break;
        case 0xc22019:  //MX25L25645G
        case 0xc22018:  //MX25L256433F
        case 0xc22017:  //MX25L256433F
        case 0xc22016:  //MX25L256433F
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x5a, 1, 0x01e0, uid, 16);
            *len = 16;
#if 0
            printf("UID:");
            for (i = 0; i < 16; i++)
            {
                printf("0x%.2x ", buf[i]);
            }
            printf("\n");
#endif
            break;
        default:
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x4b, 4, -1, uid, 16);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, 0x1000, &temp, 1);
            *len = 16;
#if 0
            printf("UID:");
            for (i = 0; i < 16; i++)
            {
                printf("0x%.2x ", buf[i]);
            }
            printf("\n");
#endif
            break;
    }

    _hudi_flash_nor_otp_wait_read_empty(reg, addr, 1000);

    return 0;
}

static int _hudi_flash_nor_otp_write_otp(sf_reg_t *reg, uint8_t *sf_addr,
                                         hudi_flash_otp_reg_e bank, unsigned int offset,
                                         unsigned char *data, unsigned int len)
{
    uint32_t id;
    uint8_t i;
    char buf[32] = {0};
    uint8_t temp = 0;
    long long address = 0;
    uint32_t otp1_start_addr = 0x1000;
    uint32_t otp2_start_addr = 0x2000;
    uint32_t otp3_start_addr = 0x3000;

    _hudi_flash_nor_otp_read_id(reg, sf_addr, buf);
    id = buf[2] | (buf[1] << 8) | (buf[0] << 16);

    //printf("id: %x\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0x00FFFFFF)
    {
        case 0x1c7119:  //EN25QE32A(OTP)
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = 0x3ff000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = 0x3fe000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = 0x3fd000 + offset;
            }
            //write enable
            uint8_t ea = 0x01;
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xc5, 0, -1, &ea, 1);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //erase otp
            _hudi_flash_otp_write_transfer_one2(reg, sf_addr, 0x44, 0, address, NULL, 0);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write otp
            _hudi_flash_otp_write_transfer_one2(reg, sf_addr, 0x42, 0, address, data, len);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            ea = 0;
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xc5, 0, -1, &ea, 1);

            break;
        case 0x1c7118:  //EN25QX128A
        case 0x1c7117:  //ME25L64A
        case 0x1c4116:  //EN25QE32A(OTP)
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = 0x3ff000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = 0x3fe000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = 0x3fd000 + offset;
            }
            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //erase otp
            _hudi_flash_otp_write_transfer_one2(reg, sf_addr, 0x44, 0, address, NULL, 0);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write otp
            _hudi_flash_otp_write_transfer_one2(reg, sf_addr, 0x42, 0, address, data, len);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);
            break;
        case 0xc22019:  //MX25L25645G
	    //enter 4byte addr
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xB7, 0, -1, NULL, 0);
            //enter otp mode
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xb1, 0, -1, NULL, 0);
            //erase otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x20, 0, 0x00, NULL, 0);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x02, 0, 0x00, data, len);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //exit otp mode
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xc1, 0, -1, NULL, 0);
	    //exit 4byte addr
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xE9, 0, -1, NULL, 0);
            break;
        case 0xc22018:  //MX25L256433F
        case 0xc22017:  //MX25L256433F
        case 0xc22016:  //MX25L256433F
            //enter otp mode
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xb1, 0, -1, NULL, 0);
            //erase otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x20, 0, 0x00, NULL, 0);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x02, 0, 0x00, data, len);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //exit otp mode
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0xc1, 0, -1, NULL, 0);
            break;
        case 0xc46017:  //GT25Q64A-S
            otp1_start_addr = 0x000;
            otp2_start_addr = 0x400;
            otp3_start_addr = 0x800;
        default:
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = otp1_start_addr + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = otp2_start_addr + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = otp3_start_addr + offset;
            }
            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //erase otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x44, 0, address, NULL, 0);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write otp
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x42, 0, address, data, len);
            //wait to finsh
            _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

            break;
    }

    return 0;
}

static inline int _hudi_flash_nor_otp_read_otp(sf_reg_t *reg, uint8_t *addr,
                                               hudi_flash_otp_reg_e bank, unsigned int offset,
                                               unsigned char *data, unsigned int len)
{
    uint32_t id;
    uint8_t i, buf[16] = {0};
    uint8_t temp;
    long long address = 0;
    uint32_t otp1_start_addr = 0x1000;
    uint32_t otp2_start_addr = 0x2000;
    uint32_t otp3_start_addr = 0x3000;

    _hudi_flash_nor_otp_read_id(reg, addr, buf);
    id = buf[2] | (buf[1] << 8) | (buf[0] << 16);

    //printf("id: %x\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0x00FFFFFF)
    {
        case 0x1c7119:  //ME25L64A
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = 0x3ff000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = 0x3fe000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = 0x3fd000 + offset;
            }
            uint8_t ea = 0x01;
            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0x06, 0, -1, NULL, 0);
            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0xc5, 0, -1, &ea, 1);

            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one2(reg, addr, 0x48, 1, address, data, len);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);

            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0x06, 0, -1, NULL, 0);
            ea = 0;
            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0xc5, 0, -1, &ea, 1);
            break;
        case 0x1c7118:  //EN25QX128A
        case 0x1c7117:  //ME25L64A
        case 0x1c4116:  //EN25QE32A(OTP)
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = 0x3ff000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = 0x3fe000 + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = 0x3fd000 + offset;
            }
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one2(reg, addr, 0x48, 1, address, data, len);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);
            break;
        case 0xc22019:  //MX25L25645G
	    //enter 4byte addr
            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0xB7, 0, -1, NULL, 0);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0xb1, 0, -1, NULL, 0);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, offset, data, len);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0xc1, 0, -1, NULL, 0);
	    //exit 4byte addr
            _hudi_flash_nor_otp_write_transfer_one(reg, addr, 0xE9, 0, -1, NULL, 0);
	    break;
        case 0xc22018:  //MX25L256433F
        case 0xc22017:  //MX25L256433F
        case 0xc22016:  //MX25L256433F
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0xb1, 0, -1, NULL, 0);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, offset, data, len);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0xc1, 0, -1, NULL, 0);
#if 0
            printf("OTP:");
            for (i = 0; i < 32; i++)
            {
                printf("0x%.2x ", otp[i]);
            }
            printf("\n");
#endif
            break;
        case 0xc46017:  //GT25Q64A-S
            otp1_start_addr = 0x000;
            otp2_start_addr = 0x400;
            otp3_start_addr = 0x800;
        default:
            if (HUDI_FLASH_OTP_REG1 == bank)
            {
                address = otp1_start_addr + offset;
            }
            else if (HUDI_FLASH_OTP_REG2 == bank)
            {
                address = otp2_start_addr + offset;
            }
            else if (HUDI_FLASH_OTP_REG3 == bank)
            {
                address = otp3_start_addr + offset;
            }
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x48, 1, address, data, len);
            _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x03, 0, address, &temp, 1);
#if 0
            printf("OTP:");
            for (i = 0; i < 32; i++)
            {
                printf("0x%.2x ", otp[i]);
            }
            printf("\n");
#endif
            break;
    }
    _hudi_flash_nor_otp_wait_read_empty(reg, addr, 1000);

    return 0;
}

static inline int _hudi_flash_nor_otp_lock_otp(sf_reg_t *reg, uint8_t *sf_addr,
                                               hudi_flash_otp_reg_e bank, bool lock)
{
    uint32_t id;
    uint8_t i, buf[16] = { 0 };
    uint8_t temp;
    uint8_t lock_otp_area = 0;
    uint8_t register_status[2] = { 0, 0 };

    if (!lock)
    {
        hudi_log(HUDI_LL_ERROR,
                 "lock = 0, no need to lock otp area\n");
        return -1;
    }

    if (HUDI_FLASH_OTP_REG1 == bank)
    {
        lock_otp_area = 0x08;
    }
    else if (HUDI_FLASH_OTP_REG2 == bank)
    {
        lock_otp_area = 0x10;
    }
    else if (HUDI_FLASH_OTP_REG3 == bank)
    {
        lock_otp_area = 0x20;
    }
    else
    {
        hudi_log(HUDI_LL_ERROR, "Incorrect reg\n");
        return -1;
    }

    _hudi_flash_nor_otp_read_id(reg, sf_addr, buf);
    id = buf[2] | (buf[1] << 8) | (buf[0] << 16);

    // printf("id: %x\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0x00FFFFFF)
    {
        case 0x1c7118:  //EN25QX128A
        case 0x1c7117:  //ME25L64A
        case 0x1c7119:  //EN25QE32A(OTP)
		if (HUDI_FLASH_OTP_REG1 == bank)
			lock_otp_area = 0x20;
		else if (HUDI_FLASH_OTP_REG2 == bank)
			lock_otp_area = 0x10;
		else if (HUDI_FLASH_OTP_REG3 == bank)
			lock_otp_area = 0x08;
		else {
			hudi_log(HUDI_LL_ERROR, "Incorrect reg\n");
			return -1;
		}
		//read register status 2
		_hudi_flash_nor_otp_read_transfer_one(reg, sf_addr, 0x35, 0, -1, &register_status[0], 1);
		//set lock otp area bit5
		register_status[0] |= lock_otp_area;
		//write enable
		_hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
		//write register status 2
		_hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x31, 0, -1, &register_status[0], 1);
		break;
        case 0xc22019:  //MX25L25645G
        case 0xc22018:  //MX25L256433F
        case 0xc22017:  //MX25L256433F
        case 0xc22016:  //MX25L256433F
            //read register status 2
            _hudi_flash_nor_otp_read_transfer_one(reg, sf_addr, 0x05, 0, -1, &register_status[0], 1);
            //read register status 2
            _hudi_flash_nor_otp_read_transfer_one(reg, sf_addr, 0x15, 0, -1, &register_status[1], 1);
            //set lock otp area bit
            register_status[1] |= 0x08;
            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write register status 2
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x31, 0, -1, &register_status[0], 2);
            break;
        default:
            //read register status 2
            _hudi_flash_nor_otp_read_transfer_one(reg, sf_addr, 0x35, 0, -1, &register_status[0], 1);
            //set lock otp area bit5
            register_status[0] |= lock_otp_area;
            //write enable
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0);
            //write register status 2
            _hudi_flash_nor_otp_write_transfer_one(reg, sf_addr, 0x31, 0, -1, &register_status[0], 1);
            break;
    }

    //wait to finish write register status 2
    _hudi_flash_nor_otp_wait_read_empty(reg, sf_addr, 1000);

    return 0;
}

/*************      nand otp function      *************/
static void _hudi_flash_nand_otp_read_transfer_one(sf_reg_t *reg, uint8_t *sf_addr,
                                                   uint8_t cmd, uint8_t dummy, long long addr,
                                                   uint8_t *rdata, uint32_t len, uint8_t addr_len)
{
    uint32_t i;
    uint8_t data = 0;
    uint8_t tmp = 0;

    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (rdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;
    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;

    if (addr >= 0)
    {
        reg->conf1.addr_hit = 0x01;
        tmp = reg->conf2.addr_bytes;
        switch (addr_len)
        {
            case 1:
                reg->conf2.addr_bytes = 0x2;
                break;
            case 2:
                reg->conf2.addr_bytes = 0x3;
                break;
            case 3:
                reg->conf2.addr_bytes = 0x0;
                break;
            case 4:
                reg->conf2.addr_bytes = 0x1;
                break;
            default:
                reg->conf2.addr_bytes = 0x3;
                break;
        }
    }
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }

    if (len > 1)
        reg->conf1.continue_read = 0x01;
    else
        reg->conf1.continue_read = 0x00;

    if (rdata != NULL)
    {
        for (i = 0; i < len; i++)
        {
            rdata[i] = sf_addr[addr + i];
        }
    }
    else if (len == 0 && rdata == NULL)
    {
        sf_addr[0] = 0;
        usleep(10000);
    }
    else
        sf_addr[addr] = 0;

    reg->conf1.continue_read = 0x00;
    if (addr >= 0)
    {
        reg->conf2.addr_bytes = tmp;
    }

    return;
}

static void _hudi_flash_nand_otp_write_transfer_one(sf_reg_t *reg, uint8_t *sf_addr,
                                                    uint8_t cmd, uint8_t dummy, long long addr,
                                                    uint8_t *wdata, uint32_t len, uint8_t addr_len)
{
    uint32_t i;
    uint8_t data = 0;
    uint8_t tmp = 0;

    //0x98
    reg->conf0.val = cmd;

    //0x99
    if (wdata)
        reg->conf1.data_hit = 0x01;
    else
        reg->conf1.data_hit = 0x00;

    if (dummy)
    {
        reg->conf1.dummy_hit = 0x01;
        reg->conf1.dummy_num = dummy - 1;
    }
    else
        reg->conf1.dummy_hit = 0x00;

    if (addr >= 0)
    {
        reg->conf1.addr_hit = 0x01;
        tmp = reg->conf2.addr_bytes;
        switch (addr_len)
        {
            case 1:
                reg->conf2.addr_bytes = 0x2;
                break;
            case 2:
                reg->conf2.addr_bytes = 0x3;
                break;
            case 3:
                reg->conf2.addr_bytes = 0x0;
                break;
            case 4:
                reg->conf2.addr_bytes = 0x1;
                break;
            default:
                reg->conf2.addr_bytes = 0x3;
                break;
        }
    }
    else
    {
        reg->conf1.addr_hit = 0x00;
        addr = 0;
    }

    if (len > 1)
        reg->conf1.continue_write = 0x01;
    else
        reg->conf1.continue_write = 0x00;

    for (i = 0; i < len; i++)
    {
        sf_addr[addr + i] = wdata[i];
    }

    if (len == 0)
    {
        sf_addr[addr] = 0;
        usleep(10000);
    }

    reg->conf1.continue_write = 0x00;
    reg->conf2.addr_bytes = tmp;
    return;
}

static void _hudi_flash_nand_otp_wait_read_empty(sf_reg_t *reg, uint8_t *addr, uint32_t timeout)
{
    uint8_t temp;
    uint32_t count = 0;

    _hudi_flash_nand_otp_read_transfer_one(reg, addr, 0x0f, 0, 0xc0, &temp, 1, 1);
    while ((temp >> 0x0) & 0x1)
    {
        _hudi_flash_nand_otp_read_transfer_one(reg, addr, 0x0f, 0, 0xc0, &temp, 1, 1);
        usleep(1000);
        count++;
        if (count == timeout)
        {
            return;
        }
    }
    return;
}

static inline void _hudi_flash_nand_otp_read_id(sf_reg_t *reg, uint8_t *addr, uint8_t *id)
{
    uint8_t temp;
    _hudi_flash_nor_otp_read_transfer_one(reg, addr, 0x9f, 1, -1, id, 2);
    _hudi_flash_nand_otp_wait_read_empty(reg, addr, 1000);
}

static int _hudi_flash_nand_otp_write_otp(sf_reg_t *reg, uint8_t *sf_addr,
                                          hudi_flash_otp_reg_e bank, unsigned int offset,
                                          unsigned char *data, unsigned int len)
{
    uint32_t id;
    uint8_t i;
    char buf [32] = {0};
    uint8_t temp = 0;
    long long address = 0;

    address = bank;

    _hudi_flash_nand_otp_read_id(reg, sf_addr, buf);
    id = (buf[1] ) | (buf[0] << 8);

    //printf("id: 0x%lx\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0xFFFF)
    {
        case 0x0B31:
        case 0x0B11:
        case 0xC851:
        default:
            /*********** otp write *************/

            //otp enable and enable ecc
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp |= 0x52;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //write enable
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0, 1);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //load otp to cache
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x02, 0,
                                                    0x00, data, len, 2);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //execute page data
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x10, 0,
                                                    (0x00 << 16) | address, NULL, 0, 3);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //otp disable and disable ecc
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp &= ~(0x52);
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            break;
    }

    return 0;
}

static inline int _hudi_flash_nand_otp_read_otp(sf_reg_t *reg, uint8_t *sf_addr,
                                                hudi_flash_otp_reg_e bank, unsigned int offset,
                                                unsigned char *data, unsigned int len)
{
    uint32_t id = 0;
    uint8_t i, buf[16] = {0};
    uint8_t temp;
    long long address = 0;

    address = bank;

    _hudi_flash_nand_otp_read_id(reg, sf_addr, buf);
    id = (buf[1] ) | (buf[0] << 8);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0xFFFF)
    {
        case 0x0B11:
        case 0xC851:
        default:
            //otp enable and enable ecc
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp = 0x52;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //read nand flash otp data to cache
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x13, 0, (0x00 << 16 ) | address, NULL, -1, 3);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //read from cache
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x03, 1, 0x00, data, len, 2);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //otp disable and disable ecc
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp &= ~0x52;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            break;
    }
    _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

    return 0;
}

static inline int _hudi_flash_nand_otp_lock_otp(sf_reg_t *reg, uint8_t *sf_addr,
                                                hudi_flash_otp_reg_e bank, bool lock)
{
    uint32_t id;
    uint8_t i;
    char buf [32] = {0};
    uint8_t temp = 0;
    long long address = 0;

    _hudi_flash_nand_otp_read_id(reg, sf_addr, buf);
    id = (buf[1] ) | (buf[0] << 8);

    //printf("id: 0x%lx\n", id);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0xFFFF)
    {
        case 0x0B11:
        case 0x0B31:
        case 0xC851:
        default:
            /*********** otp lock *************/
            //write enable
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x06, 0, -1, NULL, 0, 1);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //otp enable and ecc lock set bit
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp |= 0xc0;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //execute page data
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x10, 0,
                                                    (0x00 << 16) | address, NULL, 0, 3);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            break;
    }

    return 0;
}

static inline int _hudi_flash_nand_otp_read_uid(sf_reg_t *reg, uint8_t *sf_addr,
                                                uint8_t *uid, unsigned int *len)
{
    uint32_t id = 0;
    uint8_t i;
    uint8_t temp;
    uint8_t buf[16] = {0};

    _hudi_flash_nand_otp_read_id(reg, sf_addr, buf);
    id = (buf[1]) | (buf[0] << 8);
    if (0 == id)
    {
        return -1;
    }

    switch (id & 0xFFFF)
    {
        case 0x0B11:
        case 0x0B31:
        case 0xC851:
            //otp enable
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp = 0x40;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //read nand flash otp data to cache
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x13, 0, (0x00 << 16 ) | 0x0000, NULL, -1, 3);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //read from cache
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x03, 1, 0x00, uid, 16, 2);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            //otp disable and disable ecc
            _hudi_flash_nand_otp_read_transfer_one(reg, sf_addr, 0x0f, 0, 0xb0, &temp, 1, 1);
            temp &= ~0x40;
            _hudi_flash_nand_otp_write_transfer_one(reg, sf_addr, 0x1f, 0, 0xb0,
                                                    &temp, 1, 1);
            //wait to finsh
            _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

            *len = 16;
            break;
        default:
            break;
    }
    _hudi_flash_nand_otp_wait_read_empty(reg, sf_addr, 1000);

    return 0;
}

#pragma GCC pop_options

int hudi_flash_otp_lock(hudi_handle handle, hudi_flash_otp_reg_e bank)
{
    hudi_flash_instance_t *inst = (hudi_flash_instance_t *)handle;
    int fd = -1;
    int fdlock = -1;

    if (!handle)
    {
        hudi_log(HUDI_LL_NOTICE, "Invalid handle\n");
    }

    hudi_flash_lock();

#ifdef __HCRTOS__
    sf_reg_t *reg = (sf_reg_t *)0xb882e000;
    uint8_t *sf_addr = (uint8_t *)0xbf000000;

    fdlock = open("/dev/sf_prodect", O_RDWR);
    if (fdlock < 0)
    {
        fdlock = open("/dev/sf_protect", O_RDWR);
        if (fdlock < 0)
        {
            perror("Open lock");
            hudi_flash_unlock();
            return -1;
        }
    }
#else
    fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
    if (fdlock < 0)
    {
        perror("Open lock");
        hudi_flash_unlock();
        return -1;
    }

    if (write(fdlock, "lock", 4) <= 0)
    {
        perror("Write lock");
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }


    fd =  open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0)
    {
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }

    sf_reg_t *reg = (sf_reg_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1882e000);
    uint8_t *sf_addr = (uint8_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1f000000);

    if (!reg || !sf_addr)
    {
        close(fdlock);
        close(fd);
        hudi_flash_unlock();
        return -1;
    }
#endif

    _hudi_flash_otp_save_st(reg);
    _hudi_flash_otp_init(reg, sf_addr);

    if (HUDI_FLASH_TYPE_NOR == inst->type)
    {
        _hudi_flash_nor_otp_lock_otp(reg, sf_addr, bank, 1);
    }
    else if (HUDI_FLASH_TYPE_NAND == inst->type)
    {
        _hudi_flash_nand_otp_lock_otp(reg, sf_addr, bank, 1);
    }

    _hudi_flash_otp_restore_st(reg);

#ifdef __HCRTOS__
    close(fdlock);
#else
    munmap((void *)reg, MAP_SIZE);
    munmap((void *)sf_addr, MAP_SIZE);

    if (fd >= 0)
    {
        close(fd);
    }
    if (write(fdlock, "unlock", 6) <= 0)
    {
        perror("Write unlock");
    }
    close(fdlock);
#endif

    hudi_flash_unlock();

    return 0;
}

int hudi_flash_otp_read(hudi_handle handle, hudi_flash_otp_reg_e bank,
                        unsigned int offset, unsigned char *data, unsigned int len)
{
    hudi_flash_instance_t *inst = (hudi_flash_instance_t *)handle;
    int fd = -1;
    int fdlock = -1;

    if (!handle)
    {
        hudi_log(HUDI_LL_NOTICE, "Invalid handle\n");
    }

    if (!data)
    {
        hudi_log(HUDI_LL_ERROR, "NULL pointer\n");
        return -1;
    }

    hudi_flash_lock();

#ifdef __HCRTOS__
    sf_reg_t *reg = (sf_reg_t *)0xb882e000;
    uint8_t *sf_addr = (uint8_t *)0xbf000000;

    fdlock = open("/dev/sf_prodect", O_RDWR);
    if (fdlock < 0)
    {
        fdlock = open("/dev/sf_protect", O_RDWR);
        if (fdlock < 0)
        {
            perror("Open lock");
            hudi_flash_unlock();
            return -1;
        }
    }
#else
    fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
    if (fdlock < 0)
    {
        perror("Open lock");
        hudi_flash_unlock();
        return -1;
    }

    if (write(fdlock, "lock", 4) <= 0)
    {
        perror("Write lock");
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }


    fd =  open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0)
    {
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }

    sf_reg_t *reg = (sf_reg_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1882e000);
    uint8_t *sf_addr = (uint8_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1f000000);

    if (!reg || !sf_addr)
    {
        close(fdlock);
        close(fd);
        hudi_flash_unlock();
        return -1;
    }
#endif

    _hudi_flash_otp_save_st(reg);
    _hudi_flash_otp_init(reg, sf_addr);

    if (HUDI_FLASH_TYPE_NOR == inst->type)
    {
        _hudi_flash_nor_otp_read_otp(reg, sf_addr, bank, offset, data, len);
    }
    else if (HUDI_FLASH_TYPE_NAND == inst->type)
    {
        _hudi_flash_nand_otp_read_otp(reg, sf_addr, bank, offset, data, len);
    }

    _hudi_flash_otp_restore_st(reg);

#ifdef __HCRTOS__
    close(fdlock);
#else
    munmap((void *)reg, MAP_SIZE);
    munmap((void *)sf_addr, MAP_SIZE);

    if (fd >= 0)
    {
        close(fd);
    }
    if (write(fdlock, "unlock", 6) <= 0)
    {
        perror("Write unlock");
    }
    close(fdlock);
#endif

    hudi_flash_unlock();

    return 0;
}

int hudi_flash_otp_write(hudi_handle handle, hudi_flash_otp_reg_e bank,
                         unsigned int offset, unsigned char *data, unsigned int len)
{
    hudi_flash_instance_t *inst = (hudi_flash_instance_t *)handle;
    int fd = -1;
    int fdlock = -1;

    if (!handle)
    {
        hudi_log(HUDI_LL_NOTICE, "Invalid handle\n");
    }

    if (!data)
    {
        hudi_log(HUDI_LL_ERROR, "NULL pointer\n");
        return -1;
    }

    hudi_flash_lock();

#ifdef __HCRTOS__
    sf_reg_t *reg = (sf_reg_t *)0xb882e000;
    uint8_t *sf_addr = (uint8_t *)0xbf000000;

    fdlock = open("/dev/sf_prodect", O_RDWR);
    if (fdlock < 0)
    {
        fdlock = open("/dev/sf_protect", O_RDWR);
        if (fdlock < 0)
        {
            perror("Open lock");
            hudi_flash_unlock();
            return -1;
        }
    }
#else
    fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
    if (fdlock < 0)
    {
        perror("Open lock");
        hudi_flash_unlock();
        return -1;
    }

    if (write(fdlock, "lock", 4) <= 0)
    {
        perror("Write lock");
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }


    fd =  open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0)
    {
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }

    sf_reg_t *reg = (sf_reg_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1882e000);
    uint8_t *sf_addr = (uint8_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1f000000);

    if (!reg || !sf_addr)
    {
        close(fdlock);
        close(fd);
        hudi_flash_unlock();
        return -1;
    }
#endif

    _hudi_flash_otp_save_st(reg);
    _hudi_flash_otp_init(reg, sf_addr);

    if (HUDI_FLASH_TYPE_NOR == inst->type)
    {
        _hudi_flash_nor_otp_write_otp(reg, sf_addr, bank, offset, data, len);
    }
    else if (HUDI_FLASH_TYPE_NAND == inst->type)
    {
        _hudi_flash_nand_otp_write_otp(reg, sf_addr, bank, offset, data, len);
    }

    _hudi_flash_otp_restore_st(reg);

#ifdef __HCRTOS__
    close(fdlock);
#else
    munmap((void *)reg, MAP_SIZE);
    munmap((void *)sf_addr, MAP_SIZE);

    if (fd >= 0)
    {
        close(fd);
    }
    if (write(fdlock, "unlock", 6) <= 0)
    {
        perror("Write unlock");
    }
    close(fdlock);
#endif

    hudi_flash_unlock();

    return 0;
}

int hudi_flash_uid_read(hudi_handle handle, unsigned char *uid, unsigned int *len)
{
    hudi_flash_instance_t *inst = (hudi_flash_instance_t *)handle;
    int fd = -1;
    int fdlock = -1;

    if (!handle)
    {
        hudi_log(HUDI_LL_NOTICE, "Invalid handle\n");
    }

    if (!uid || !len)
    {
        hudi_log(HUDI_LL_ERROR, "NULL pointer\n");
        return -1;
    }

    hudi_flash_lock();

#ifdef __HCRTOS__
    sf_reg_t *reg = (sf_reg_t *)0xb882e000;
    uint8_t *sf_addr = (uint8_t *)0xbf000000;

    fdlock = open("/dev/sf_prodect", O_RDWR);
    if (fdlock < 0)
    {
        fdlock = open("/dev/sf_protect", O_RDWR);
        if (fdlock < 0)
        {
            perror("Open lock");
            hudi_flash_unlock();
            return -1;
        }
    }
#else
    fdlock = open("/sys/devices/platform/soc/1882e000.spi/protect", O_RDWR);
    if (fdlock < 0)
    {
        perror("Open lock");
        hudi_flash_unlock();
        return -1;
    }

    if (write(fdlock, "lock", 4) <= 0)
    {
        perror("Write lock");
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }


    fd =  open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0)
    {
        close(fdlock);
        hudi_flash_unlock();
        return -1;
    }

    sf_reg_t *reg = (sf_reg_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1882e000);
    uint8_t *sf_addr = (uint8_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1f000000);

    if (!reg || !sf_addr)
    {
        close(fdlock);
        close(fd);
        hudi_flash_unlock();
        return -1;
    }
#endif

    _hudi_flash_otp_save_st(reg);
    _hudi_flash_otp_init(reg, sf_addr);

    if (HUDI_FLASH_TYPE_NOR == inst->type)
    {
        _hudi_flash_nor_otp_read_uid(reg, sf_addr, uid, len);
    }
    else if (HUDI_FLASH_TYPE_NAND == inst->type)
    {
        _hudi_flash_nand_otp_read_uid(reg, sf_addr, uid, len);
    }

    _hudi_flash_otp_restore_st(reg);

#ifdef __HCRTOS__
    close(fdlock);
#else
    munmap((void *)reg, MAP_SIZE);
    munmap((void *)sf_addr, MAP_SIZE);

    if (fd >= 0)
    {
        close(fd);
    }
    if (write(fdlock, "unlock", 6) <= 0)
    {
        perror("Write unlock");
    }
    close(fdlock);
#endif

    hudi_flash_unlock();

    return 0;
}
