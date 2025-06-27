#ifndef _RN6752_INITABLE_CVBS_PAL_H_
#define _RN6752_INITABLE_CVBS_PAL_H_

#include "../dvp.h"



struct regval_list RN675_init_cfg_cvbs_pal[] = {
// 720H@50, 27MHz, BT656 output
// Slave address is 0x58
// Register, data

// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
//0xD2, 0x85, // disable auto clock detect
//0xD6, 0x37, // 27MHz default
//0xD8, 0x18, // switch to 26MHz clock
//delay(100), // delay 100ms
 
{0x81, 0x01}, // turn on video decoder
{0xA3, 0x00}, // enable 72MHz sampling
{0xDB, 0x8F}, // internal use*
{0xFF, 0x00}, // switch to ch0 (default; optional)
{0x2C, 0x30}, // select sync slice points
{0x50, 0x00}, // 720H resolution select for BT.601
{0x56, 0x00}, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
{0x63, 0x09}, // filter control
{0x59, 0x00}, // extended register access
{0x5A, 0x00}, // data for extended register
{0x58, 0x01}, // enable extended register write
{0x07, 0x22}, // TV_PAL format
{0x2F, 0x14}, // internal use
{0x5E, 0x03}, // disable H-scaling control
{0x5B, 0x00}, //
{0x3A, 0x04}, // no channel information insertion; invert VBLK for frame valid
{0x3E, 0x32}, // AVID & VBLK out for BT.601
{0x40, 0x04}, // no channel information insertion; invert VBLK for frame valid
{0x46, 0x23}, // AVID & VBLK out for BT.601
{0x28, 0x92}, // cropping
{0x00, 0x00}, // internal use*
{0x2D, 0xF2}, // cagc adjust
{0x0D, 0x20}, // cagc initial value 
{0x05, 0x00}, // sharpness
{0x04, 0x80}, // hue
{0x11, 0x03},
{0x37, 0x33},
{0x61, 0x6C},

{0xDF, 0xFF}, // enable 720H format
{0x8E, 0x00}, // single channel output for VP
{0x8F, 0x00}, // 720H mode for VP
{0x8D, 0x31}, // enable VP out
{0x89, 0x00}, // select 27MHz for SCLK
{0x88, 0xC1}, // enable SCLK out
{0x81, 0x01}, // turn on video decoder

{0x96, 0x00}, // select AVID & VBLK as status indicator
{0x97, 0x0B}, // enable status indicator out on AVID,VBLK & VSYNC 
{0x98, 0x00}, // video timing pin status
{0x9A, 0x40}, // select AVID & VBLK as status indicator 
{0x9B, 0xE1}, // enable status indicator out on HSYNC
{0x9C, 0x00}, // video timing pin status
{0xFF, 0xFF},
};


#endif
