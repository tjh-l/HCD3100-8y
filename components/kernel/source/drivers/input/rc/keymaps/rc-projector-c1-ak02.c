// SPDX-License-Identifier: GPL-2.0+
// Keytable for rc_projector_c1_ak02_ Remote Controller
//
// Copyright © 2021 HiChip Semiconductor Co., Ltd.
//              http://www.hichiptech.com

#include <kernel/module.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/rc-proto.h>
#include "../rc-map.h"

static struct rc_map_table rc_projector_c1_ak02[] = {
#if 1
{0x14,KEY_POWER},
{0x93,KEY_MUTE},
{0x5c,KEY_EPG},//SETTING
{0x03,KEY_UP},
{0x0e,KEY_LEFT},
{0x1a,KEY_RIGHT},
{0x02,KEY_DOWN},
{0x07,KEY_OK},
{0x98,KEY_VOLUMEDOWN},
{0x82,KEY_VOLUMEUP},
{0x13,KEY_MENU},//SOURCE
{0x48,KEY_EXIT},
//{0x93,KEY_PLAY},//play pause
{0x58,KEY_BACK},//fb    
{0x0b,KEY_FORWARD},//ff
{0x01,KEY_HOME},//ff

#endif

#if 0 // A8
	{ 0xa8, KEY_POWER },
	{ 0x88, KEY_MUTE },
	{ 0x91, KEY_EPG },//SETTING
	{ 0x95, KEY_UP    },
	{ 0x9b, KEY_LEFT  },
	{ 0x99, KEY_RIGHT },
	{ 0x9a, KEY_DOWN  },
	{ 0x9e, KEY_OK },
	{ 0x9c, KEY_VOLUMEDOWN },
	{ 0x8c, KEY_VOLUMEUP },
	{ 0x97, KEY_MENU },//SOURCE
	{ 0xa4, KEY_EXIT },
	{ 0x93, KEY_PLAY},	//play pause
//	{ 0x98, KEY_LEFTSHIFT },	//fb	
//	{ 0x82, KEY_RIGHTSHIFT },	//ff
//	{ 0x98, KEY_FAV_DOWN },	//fb	
//	{ 0x82, KEY_FAV_UP },	//ff
	{ 0x98, KEY_FORWARD },
	{ 0x82, KEY_BACK },		
#endif

#if 1 //test2 0x21DF
	{ 0xdf1c, KEY_POWER },
	{ 0xdf08, KEY_F12 },//KEY_MUTE
	{ 0xdf01, KEY_F11 },//KEY_MENU
	{ 0xdf5b, KEY_FORWARD },//
	{ 0xdf41, KEY_BACK },//
	{ 0xdf1a, KEY_F9 },//KEY_UP
	{ 0xdf47, KEY_F8 },//KEY_LEFT
	{ 0xdf06, KEY_F7 },//KEY_OK
	{ 0xdf07, KEY_F6 },//KEY_RIGHT
	{ 0xdf48, KEY_F5 },//KEY_DOWN
	{ 0xdf18, KEY_F4 },//KEY_HOME
	{ 0xdf03, KEY_F3 },//KEY_EPG
	{ 0xdf0a, KEY_F2 },//KEY_EXIT
	{ 0xdf4f, KEY_F1 },//KEY_VOLUMEDOWN
	{ 0xdf4b, KEY_F10 },//KEY_VOLUMEUP
#endif
};

static struct rc_map_list rc_projector_c1_ak02_map = {
	.map = {
		.scan     = rc_projector_c1_ak02,
		.size     = ARRAY_SIZE(rc_projector_c1_ak02),
		.rc_proto = RC_PROTO_NEC,
		.name     = "rc-projector-c1-ak02",
	}
};

static int init_rc_map_rc_projector_c1_ak02(void)
{
	return rc_map_register(&rc_projector_c1_ak02_map);
}

module_system(rc_map_rc_projector_c1_ak02, init_rc_map_rc_projector_c1_ak02, NULL, 0)
