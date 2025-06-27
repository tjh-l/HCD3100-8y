/*
2020-12-07
1.first release

2020-12-10
1.update AHD new setting to save power
2.add CVBS 720H support

2024-02-21
Add HD50/60
*/

#include <kernel/types.h>
#include <stdio.h>
#include "../dvp_i2c.h"
#include "tp2804_reg.h"

void tp9951_write_reg(unsigned char reg , unsigned char data)
{
    int ret = 0;
    uint8_t array[2];

    array[0] = reg;
    array[1] = data;

    ret = dvp_i2c_write(array , 2);
    if (ret < 0) {
        printf("%s %d fail\n" , __FUNCTION__ , __LINE__);
    }
    //printf("%s cmd = 0x%x data=0x%x\n" , __FUNCTION__ , cmd , data);
    return;
}

void tp28xx_byte_write(unsigned char reg , unsigned char data)
{
    tp9951_write_reg(reg , data);
}

void tp2860_write_reg(unsigned char reg , unsigned char data)
{
    tp9951_write_reg(reg , data);
}

void tp28xx_write_reg(unsigned char reg , unsigned char data)
{
    tp9951_write_reg(reg , data);
}

unsigned char tp2860_read_reg(unsigned char reg)
{
    int ret = 0;
    uint8_t data = 0;

    ret = dvp_i2c_write(&reg , 1);

    if (ret < 0) {
        printf("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return ret;
    }

    ret = dvp_i2c_read(&data , 1);
    if (ret < 0) {
        return 0;
    }
    return data;
}

/////////////////////////////////
void TP9951_dvp_out(unsigned char fmt , unsigned char std)
{
    //mipi setting
    char tmp = 0;
    tp28xx_byte_write(0x40 , 0x08); //select mipi page

    if (FHD30 == fmt || FHD25 == fmt || FHD275 == fmt || FHD28 == fmt || FHD_X3C == fmt || HD50 == fmt || HD60 == fmt) {
        tp28xx_byte_write(0x12 , 0x54);
        tp28xx_byte_write(0x13 , 0xef);
        tp28xx_byte_write(0x14 , 0x41);
        tp28xx_byte_write(0x15 , 0x02);
        tp28xx_byte_write(0x40 , 0x00);
    } else if (HD30 == fmt || HD25 == fmt || HD275 == fmt) {
        tp28xx_byte_write(0x12 , 0x54);
        tp28xx_byte_write(0x13 , 0xef);
        tp28xx_byte_write(0x14 , 0x41);
        tp28xx_byte_write(0x15 , 0x12);
        tp28xx_byte_write(0x40 , 0x00);
    } else if (UVGA25 == fmt || UVGA30 == fmt) {
        tp28xx_byte_write(0x13 , 0x0f);
        tp28xx_byte_write(0x12 , 0x5f);

        tp28xx_byte_write(0x14 , 0x41);
        tp28xx_byte_write(0x15 , 0x02);
        tp28xx_byte_write(0x40 , 0x00);
    } else if (F_UVGA30 == fmt) {
        tp28xx_byte_write(0x13 , 0x0f);
        tp28xx_byte_write(0x12 , 0x5e);

        tp28xx_byte_write(0x14 , 0x41);
        tp28xx_byte_write(0x15 , 0x02);
        tp28xx_byte_write(0x40 , 0x00);
    }

    tp2860_write_reg(0x40 , 0x00); //back to decoder page
    tmp = tp2860_read_reg(0x06); //PLL reset
    tp2860_write_reg(0x06 , 0x80 | tmp);

    tp2860_write_reg(0x40 , 0x08); //back to mipi page

    tmp = tp2860_read_reg(0x14); //PLL reset
    tp2860_write_reg(0x14 , 0x80 | tmp);
    tp2860_write_reg(0x14 , tmp);

    tp28xx_byte_write(0x40 , 0x00); //back to decoder page
}
/////////////////////////////////
//ch: video channel
//fmt: PAL/NTSC/HD25/HD30
//std: STD_TVI/STD_HDA
//sample: TP9951_sensor_init(VIN1,HD30,STD_TVI); //video is TVI 720p30 from Vin1
////////////////////////////////
void TP9951_sensor_init(unsigned char ch , unsigned char fmt , unsigned char std)
{
    tp9951_write_reg(0x40 , 0x00); //select decoder page
    tp28xx_write_reg(0x06 , 0x12); //default value
    tp9951_write_reg(0x42 , 0x00);	//common setting for all format
    tp9951_write_reg(0x4c , 0x43);	//common setting for all format
    tp9951_write_reg(0x4e , 0x1d); //common setting for dvp output
    tp9951_write_reg(0x54 , 0x04); //common setting for dvp output

    tp9951_write_reg(0xf6 , 0x00);	//common setting for all format
    tp9951_write_reg(0xf7 , 0x44); //common setting for dvp output
    tp9951_write_reg(0xfa , 0x00); //common setting for dvp output
    tp9951_write_reg(0x1b , 0x01); //common setting for dvp output
    tp9951_write_reg(0x41 , ch);		//video MUX select

    tp9951_write_reg(0x40 , 0x08);	//common setting for all format
    tp9951_write_reg(0x13 , 0xef); //common setting for dvp output
    tp9951_write_reg(0x14 , 0x41); //common setting for dvp output
    tp9951_write_reg(0x15 , 0x02); //common setting for dvp output

    tp9951_write_reg(0x40 , 0x00); //select decoder page

    TP9951_dvp_out(fmt , std);

    if (HD25 == fmt) {
        tp9951_write_reg(0x02 , 0xca);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x13);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x13);
        tp9951_write_reg(0x16 , 0x15);
        tp9951_write_reg(0x17 , 0x00);
        tp9951_write_reg(0x18 , 0x19);
        tp9951_write_reg(0x19 , 0xd0);
        tp9951_write_reg(0x1a , 0x25);
        tp9951_write_reg(0x1c , 0x07);  //1280*720, 25fps
        tp9951_write_reg(0x1d , 0xbc);  //1280*720, 25fps

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x25);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x18);

        if (STD_HDA == std)  //AHD720p25 extra
        {
            tp28xx_byte_write(0x02 , 0xce);

            tp28xx_byte_write(0x0d , 0x71);

            tp28xx_byte_write(0x18 , 0x1b);

            tp28xx_byte_write(0x20 , 0x40);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x01);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x5a);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0x9e);
            tp28xx_byte_write(0x31 , 0x20);
            tp28xx_byte_write(0x32 , 0x10);
            tp28xx_byte_write(0x33 , 0x90);
        }
    } else if (HD30 == fmt) {
        tp9951_write_reg(0x02 , 0xca);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x13);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x13);
        tp9951_write_reg(0x16 , 0x15);
        tp9951_write_reg(0x17 , 0x00);
        tp9951_write_reg(0x18 , 0x19);
        tp9951_write_reg(0x19 , 0xd0);
        tp9951_write_reg(0x1a , 0x25);
        tp9951_write_reg(0x1c , 0x06);  //1280*720, 30fps
        tp9951_write_reg(0x1d , 0x72);  //1280*720, 30fps

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x25);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x18);

        if (STD_HDA == std) //AHD720p30 extra
        {
            tp28xx_byte_write(0x02 , 0xce);

            tp28xx_byte_write(0x0d , 0x70);

            tp28xx_byte_write(0x18 , 0x1b);

            tp28xx_byte_write(0x20 , 0x40);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x01);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x5a);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0x9d);
            tp28xx_byte_write(0x31 , 0xca);
            tp28xx_byte_write(0x32 , 0x01);
            tp28xx_byte_write(0x33 , 0xd0);
        }
    } else if (HD275 == fmt)		//720P27.5
    {
        tp9951_write_reg(0x02 , 0xca);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x13);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x13);
        tp9951_write_reg(0x16 , 0x15);
        tp9951_write_reg(0x17 , 0x00);
        tp9951_write_reg(0x18 , 0x19);
        tp9951_write_reg(0x19 , 0xd0);
        tp9951_write_reg(0x1a , 0x25);
        tp9951_write_reg(0x1c , 0x07);  //1280*720, 27.5fps
        tp9951_write_reg(0x1d , 0x08);  //1280*720, 27.5fps

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x25);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x18);

        if (STD_HDA == std) //AHD720p30 extra
        {
        }
    } else if (FHD30 == fmt) {
        tp9951_write_reg(0x02 , 0xc8);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x03);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x03);
        tp9951_write_reg(0x16 , 0xd2);
        tp9951_write_reg(0x17 , 0x80);
        tp9951_write_reg(0x18 , 0x29);
        tp9951_write_reg(0x19 , 0x38);
        tp9951_write_reg(0x1a , 0x47);
        tp9951_write_reg(0x1c , 0x08);  //1920*1080, 30fps
        tp9951_write_reg(0x1d , 0x98);  //

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x05);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x1C);

        if (STD_HDA == std) //AHD1080p30 extra
        {
            tp28xx_byte_write(0x02 , 0xcc);

            tp28xx_byte_write(0x0d , 0x72);

            tp28xx_byte_write(0x15 , 0x01);
            tp28xx_byte_write(0x16 , 0xf0);
            tp28xx_byte_write(0x18 , 0x2a);

            tp28xx_byte_write(0x20 , 0x38);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x0d);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x54);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0xa5);
            tp28xx_byte_write(0x31 , 0x95);
            tp28xx_byte_write(0x32 , 0xe0);
            tp28xx_byte_write(0x33 , 0x60);
        }
    } else if (FHD25 == fmt) {
        tp9951_write_reg(0x02 , 0xc8);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x03);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x03);
        tp9951_write_reg(0x16 , 0xd2);
        tp9951_write_reg(0x17 , 0x80);
        tp9951_write_reg(0x18 , 0x29);
        tp9951_write_reg(0x19 , 0x38);
        tp9951_write_reg(0x1a , 0x47);

        tp9951_write_reg(0x1c , 0x0a);  //1920*1080, 25fps
        tp9951_write_reg(0x1d , 0x50);  //

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x05);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x1C);

        if (STD_HDA == std)  //AHD1080p25 extra
        {
            tp28xx_byte_write(0x02 , 0xcc);

            tp28xx_byte_write(0x0d , 0x73);

            tp28xx_byte_write(0x15 , 0x01);
            tp28xx_byte_write(0x16 , 0xf0);
            tp28xx_byte_write(0x18 , 0x2a);

            tp28xx_byte_write(0x20 , 0x3c);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x0d);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x54);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0xa5);
            tp28xx_byte_write(0x31 , 0x86);
            tp28xx_byte_write(0x32 , 0xfb);
            tp28xx_byte_write(0x33 , 0x60);
        }
    } else if (FHD275 == fmt)	//TVI 1080p27.5
    {
        tp9951_write_reg(0x02 , 0xc8);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x03);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x13);
        tp9951_write_reg(0x16 , 0x88);
        tp9951_write_reg(0x17 , 0x80);
        tp9951_write_reg(0x18 , 0x29);
        tp9951_write_reg(0x19 , 0x38);
        tp9951_write_reg(0x1a , 0x47);

        tp9951_write_reg(0x1c , 0x09);  //1920*1080, 27.5fps
        tp9951_write_reg(0x1d , 0x60);  //

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x05);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x1C);
        if (STD_HDA == std) {
#if 0 // op2
            tp28xx_byte_write(0x02 , 0xcc);

            tp28xx_byte_write(0x0d , 0x73);

            tp28xx_byte_write(0x15 , 0x11);
            tp28xx_byte_write(0x16 , 0xd2);
            tp28xx_byte_write(0x18 , 0x2a);

            tp28xx_byte_write(0x20 , 0x38);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x0d);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x54);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0xa6);
            tp28xx_byte_write(0x31 , 0x14);
            tp28xx_byte_write(0x32 , 0x7a);
            tp28xx_byte_write(0x33 , 0xe0);
#else	//op1
            tp28xx_byte_write(0x02 , 0xc8);

            tp28xx_byte_write(0x0d , 0x50);

            tp28xx_byte_write(0x15 , 0x11);
            tp28xx_byte_write(0x16 , 0xd2);
            tp28xx_byte_write(0x18 , 0x2a);

            tp28xx_byte_write(0x20 , 0x38);
            tp28xx_byte_write(0x21 , 0x46);

            tp28xx_byte_write(0x25 , 0xfe);
            tp28xx_byte_write(0x26 , 0x0d);

            tp28xx_byte_write(0x2c , 0x3a);
            tp28xx_byte_write(0x2d , 0x54);
            tp28xx_byte_write(0x2e , 0x40);

            tp28xx_byte_write(0x30 , 0x29);
            tp28xx_byte_write(0x31 , 0x85);
            tp28xx_byte_write(0x32 , 0x1e);
            tp28xx_byte_write(0x33 , 0xb0);

#endif
        }
    } else if (FHD28 == fmt)	//TVI 1080p28
    {
        tp9951_write_reg(0x02 , 0xc8);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x03);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x03);
        tp9951_write_reg(0x16 , 0xd2);
        tp9951_write_reg(0x17 , 0x80);
        tp9951_write_reg(0x18 , 0x79);
        tp9951_write_reg(0x19 , 0x38);
        tp9951_write_reg(0x1a , 0x47);

        tp9951_write_reg(0x1c , 0x08);  //1920*1080, 27.5fps
        tp9951_write_reg(0x1d , 0x98);  //

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x14);
        tp9951_write_reg(0x36 , 0xb5);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x1C);
    } else if (FHD_X3C == fmt)	//TVI 1080p25  2475x1205
    {
        tp9951_write_reg(0x02 , 0xc8);
        tp9951_write_reg(0x07 , 0xc0);
        tp9951_write_reg(0x0b , 0xc0);
        tp9951_write_reg(0x0c , 0x03);
        tp9951_write_reg(0x0d , 0x50);

        tp9951_write_reg(0x15 , 0x13);
        tp9951_write_reg(0x16 , 0xE8);
        tp9951_write_reg(0x17 , 0x80);
        tp9951_write_reg(0x18 , 0x54);
        tp9951_write_reg(0x19 , 0x38);
        tp9951_write_reg(0x1a , 0x47);

        tp9951_write_reg(0x1c , 0x09);
        tp9951_write_reg(0x1d , 0xAB);

        tp9951_write_reg(0x20 , 0x30);
        tp9951_write_reg(0x21 , 0x84);
        tp9951_write_reg(0x22 , 0x36);
        tp9951_write_reg(0x23 , 0x3c);

        tp9951_write_reg(0x2b , 0x60);
        tp9951_write_reg(0x2c , 0x2a);
        tp9951_write_reg(0x2d , 0x30);
        tp9951_write_reg(0x2e , 0x70);

        tp9951_write_reg(0x30 , 0x48);
        tp9951_write_reg(0x31 , 0xbb);
        tp9951_write_reg(0x32 , 0x2e);
        tp9951_write_reg(0x33 , 0x90);

        tp9951_write_reg(0x35 , 0x14);
        tp9951_write_reg(0x36 , 0xb5);
        tp9951_write_reg(0x38 , 0x00);
        tp9951_write_reg(0x39 , 0x1C);
    } else if (UVGA25 == fmt) //TVI960P25
    {
        tp28xx_write_reg(0x02 , 0xca);
        tp28xx_write_reg(0x07 , 0xc0);
        tp28xx_write_reg(0x0b , 0xc0);
        tp28xx_write_reg(0x0c , 0x13);
        tp28xx_write_reg(0x0d , 0x50);

        tp28xx_write_reg(0x15 , 0x13);
        tp28xx_write_reg(0x16 , 0x16);
        tp28xx_write_reg(0x17 , 0x00);
        tp28xx_write_reg(0x18 , 0xa0);
        tp28xx_write_reg(0x19 , 0xc0);
        tp28xx_write_reg(0x1a , 0x35);
        tp28xx_write_reg(0x1c , 0x07);  //
        tp28xx_write_reg(0x1d , 0xbc);  //

        tp28xx_write_reg(0x20 , 0x30);
        tp28xx_write_reg(0x21 , 0x84);
        tp28xx_write_reg(0x22 , 0x36);
        tp28xx_write_reg(0x23 , 0x3c);

        tp28xx_write_reg(0x26 , 0x01);

        tp28xx_write_reg(0x2b , 0x60);
        tp28xx_write_reg(0x2c , 0x0a);
        tp28xx_write_reg(0x2d , 0x30);
        tp28xx_write_reg(0x2e , 0x70);

        tp28xx_write_reg(0x30 , 0x48);
        tp28xx_write_reg(0x31 , 0xba);
        tp28xx_write_reg(0x32 , 0x2e);
        tp28xx_write_reg(0x33 , 0x90);

        tp28xx_write_reg(0x35 , 0x14);
        tp28xx_write_reg(0x36 , 0x65);
        tp28xx_write_reg(0x38 , 0x00);
        tp28xx_write_reg(0x39 , 0x1c);
    } else if (UVGA30 == fmt) //TVI960P30
    {
        tp28xx_write_reg(0x02 , 0xca);
        tp28xx_write_reg(0x07 , 0xc0);
        tp28xx_write_reg(0x0b , 0xc0);
        tp28xx_write_reg(0x0c , 0x13);
        tp28xx_write_reg(0x0d , 0x50);

        tp28xx_write_reg(0x15 , 0x13);
        tp28xx_write_reg(0x16 , 0x16);
        tp28xx_write_reg(0x17 , 0x00);
        tp28xx_write_reg(0x18 , 0xa0);
        tp28xx_write_reg(0x19 , 0xc0);
        tp28xx_write_reg(0x1a , 0x35);
        tp28xx_write_reg(0x1c , 0x06);  //
        tp28xx_write_reg(0x1d , 0x72);  //

        tp28xx_write_reg(0x20 , 0x30);
        tp28xx_write_reg(0x21 , 0x84);
        tp28xx_write_reg(0x22 , 0x36);
        tp28xx_write_reg(0x23 , 0x3c);

        tp28xx_write_reg(0x26 , 0x01);

        tp28xx_write_reg(0x2b , 0x60);
        tp28xx_write_reg(0x2c , 0x0a);
        tp28xx_write_reg(0x2d , 0x30);
        tp28xx_write_reg(0x2e , 0x70);

        tp28xx_write_reg(0x30 , 0x43);
        tp28xx_write_reg(0x31 , 0x3b);
        tp28xx_write_reg(0x32 , 0x79);
        tp28xx_write_reg(0x33 , 0x90);

        tp28xx_write_reg(0x35 , 0x14);
        tp28xx_write_reg(0x36 , 0x65);
        tp28xx_write_reg(0x38 , 0x00);
        tp28xx_write_reg(0x39 , 0x1c);
    } else if (F_UVGA30 == fmt) //FH 960P30
    {
        tp28xx_write_reg(0x02 , 0xcc);
        tp28xx_write_reg(0x07 , 0xc0);
        tp28xx_write_reg(0x0b , 0xc0);
        tp28xx_write_reg(0x0c , 0x03);
        tp28xx_write_reg(0x0d , 0x76);
        tp28xx_write_reg(0x0e , 0x16);

        tp28xx_write_reg(0x15 , 0x13);
        tp28xx_write_reg(0x16 , 0x8f);
        tp28xx_write_reg(0x17 , 0x00);
        tp28xx_write_reg(0x18 , 0x23);
        tp28xx_write_reg(0x19 , 0xc0);
        tp28xx_write_reg(0x1a , 0x35);
        tp28xx_write_reg(0x1c , 0x07);  //
        tp28xx_write_reg(0x1d , 0x08);  //

        tp28xx_write_reg(0x20 , 0x60);
        tp28xx_write_reg(0x21 , 0x84);
        tp28xx_write_reg(0x22 , 0x36);
        tp28xx_write_reg(0x23 , 0x3c);

        tp28xx_write_reg(0x26 , 0x05);

        tp28xx_write_reg(0x2b , 0x60);
        tp28xx_write_reg(0x2c , 0x2a);
        tp28xx_write_reg(0x2d , 0x70);
        tp28xx_write_reg(0x2e , 0x50);

        tp28xx_write_reg(0x30 , 0x7f);
        tp28xx_write_reg(0x31 , 0x49);
        tp28xx_write_reg(0x32 , 0xf4);
        tp28xx_write_reg(0x33 , 0x90);

        tp28xx_write_reg(0x35 , 0x13);
        tp28xx_write_reg(0x36 , 0xe8);
        tp28xx_write_reg(0x38 , 0x00);
        tp28xx_write_reg(0x39 , 0x88);
    } else if (HD50 == fmt) {
        tp28xx_write_reg(0x02 , 0xca);
        tp28xx_write_reg(0x07 , 0xc0);
        tp28xx_write_reg(0x0b , 0xc0);
        tp28xx_write_reg(0x0c , 0x13);
        tp28xx_write_reg(0x0d , 0x50);

        tp28xx_write_reg(0x15 , 0x13);
        tp28xx_write_reg(0x16 , 0x15);
        tp28xx_write_reg(0x17 , 0x00);
        tp28xx_write_reg(0x18 , 0x19);
        tp28xx_write_reg(0x19 , 0xd0);
        tp28xx_write_reg(0x1a , 0x25);
        tp28xx_write_reg(0x1c , 0x07);  //1280*720,
        tp28xx_write_reg(0x1d , 0xbc);  //1280*720, 50fps

        tp28xx_write_reg(0x20 , 0x30);
        tp28xx_write_reg(0x21 , 0x84);
        tp28xx_write_reg(0x22 , 0x36);
        tp28xx_write_reg(0x23 , 0x3c);

        tp28xx_write_reg(0x2b , 0x60);
        tp28xx_write_reg(0x2c , 0x2a);
        tp28xx_write_reg(0x2d , 0x30);
        tp28xx_write_reg(0x2e , 0x70);

        tp28xx_write_reg(0x30 , 0x48);
        tp28xx_write_reg(0x31 , 0xbb);
        tp28xx_write_reg(0x32 , 0x2e);
        tp28xx_write_reg(0x33 , 0x90);

        tp28xx_write_reg(0x35 , 0x05);
        tp28xx_write_reg(0x38 , 0x00);
        tp28xx_write_reg(0x39 , 0x1c);

        if (STD_HDA == std)      //subcarrier=24M
        {
            tp28xx_write_reg(0x02 , 0xce);
            tp28xx_write_reg(0x05 , 0x01);
            tp28xx_write_reg(0x0d , 0x76);
            tp28xx_write_reg(0x0e , 0x0a);
            tp28xx_write_reg(0x14 , 0x00);
            tp28xx_write_reg(0x15 , 0x13);
            tp28xx_write_reg(0x16 , 0x1a);
            tp28xx_write_reg(0x18 , 0x1b);

            tp28xx_write_reg(0x20 , 0x40);

            tp28xx_write_reg(0x26 , 0x01);

            tp28xx_write_reg(0x2c , 0x3a);
            tp28xx_write_reg(0x2d , 0x54);
            tp28xx_write_reg(0x2e , 0x50);

            tp28xx_write_reg(0x30 , 0xa5);
            tp28xx_write_reg(0x31 , 0x9f);
            tp28xx_write_reg(0x32 , 0xce);
            tp28xx_write_reg(0x33 , 0x60);
        }
    } else if (HD60 == fmt) {
        tp28xx_write_reg(0x02 , 0xca);
        tp28xx_write_reg(0x07 , 0xc0);
        tp28xx_write_reg(0x0b , 0xc0);
        tp28xx_write_reg(0x0c , 0x13);
        tp28xx_write_reg(0x0d , 0x50);

        tp28xx_write_reg(0x15 , 0x13);
        tp28xx_write_reg(0x16 , 0x15);
        tp28xx_write_reg(0x17 , 0x00);
        tp28xx_write_reg(0x18 , 0x19);
        tp28xx_write_reg(0x19 , 0xd0);
        tp28xx_write_reg(0x1a , 0x25);
        tp28xx_write_reg(0x1c , 0x06);  //1280*720,
        tp28xx_write_reg(0x1d , 0x72);  //1280*720, 60fps

        tp28xx_write_reg(0x20 , 0x30);
        tp28xx_write_reg(0x21 , 0x84);
        tp28xx_write_reg(0x22 , 0x36);
        tp28xx_write_reg(0x23 , 0x3c);

        tp28xx_write_reg(0x2b , 0x60);
        tp28xx_write_reg(0x2c , 0x2a);
        tp28xx_write_reg(0x2d , 0x30);
        tp28xx_write_reg(0x2e , 0x70);

        tp28xx_write_reg(0x30 , 0x48);
        tp28xx_write_reg(0x31 , 0xbb);
        tp28xx_write_reg(0x32 , 0x2e);
        tp28xx_write_reg(0x33 , 0x90);

        tp28xx_write_reg(0x35 , 0x05);
        tp28xx_write_reg(0x38 , 0x00);
        tp28xx_write_reg(0x39 , 0x1c);
#if 0
        if (STD_HDA == std)		////subcarrier=11M
        {
            tp28xx_write_reg(0x02 , 0xce);
            tp28xx_write_reg(0x05 , 0xf9);
            tp28xx_write_reg(0x0d , 0x76);
            tp28xx_write_reg(0x0e , 0x03);
            tp28xx_write_reg(0x14 , 0x00);
            tp28xx_write_reg(0x15 , 0x13);
            tp28xx_write_reg(0x16 , 0x41);
            tp28xx_write_reg(0x18 , 0x1b);

            tp28xx_write_reg(0x20 , 0x50);
            tp28xx_write_reg(0x21 , 0x84);

            tp28xx_write_reg(0x25 , 0xff);
            tp28xx_write_reg(0x26 , 0x0d);

            tp28xx_write_reg(0x2c , 0x3a);
            tp28xx_write_reg(0x2d , 0x68);
            tp28xx_write_reg(0x2e , 0x60);

            tp28xx_write_reg(0x30 , 0x4e);
            tp28xx_write_reg(0x31 , 0xf8);
            tp28xx_write_reg(0x32 , 0xdc);
            tp28xx_write_reg(0x33 , 0xf0);
        }
#else
        if (STD_HDA == std)		////subcarrier=22M
        {
            tp28xx_write_reg(0x02 , 0xce);
            tp28xx_write_reg(0x05 , 0x55);
            tp28xx_write_reg(0x0d , 0x76);
            tp28xx_write_reg(0x0e , 0x08);
            tp28xx_write_reg(0x14 , 0x00);
            tp28xx_write_reg(0x15 , 0x13);
            tp28xx_write_reg(0x16 , 0x25);
            tp28xx_write_reg(0x18 , 0x1b);

            tp28xx_write_reg(0x20 , 0x50);
            tp28xx_write_reg(0x21 , 0x84);

            tp28xx_write_reg(0x25 , 0xff);
            tp28xx_write_reg(0x26 , 0x01);

            tp28xx_write_reg(0x2c , 0x2a);
            tp28xx_write_reg(0x2d , 0x54);
            tp28xx_write_reg(0x2e , 0x60);

            tp28xx_write_reg(0x30 , 0xa5);
            tp28xx_write_reg(0x31 , 0x8b);
            tp28xx_write_reg(0x32 , 0xf2);
            tp28xx_write_reg(0x33 , 0x60);
        }
#endif
    }
}

void tp2804_debug(void)
{
    uint8_t i;
    unsigned char data = 0;

    for (i = 0; i < 0xFF; i++) {
        data = tp2860_read_reg(i);
        printf("i:0x%x value:0x%x\n" , i , data);
    }
}