// SPDX-License-Identifier: GPL-2.0+
// Keytable for hcdemo Remote Controller
//
// Copyright © 2021 HiChip Semiconductor Co., Ltd.
//              http://www.hichiptech.com

#include <kernel/module.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/rc-proto.h>
#include "../rc-map.h"

static struct rc_map_table hcdemo[] = {
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
//	 { 0x98, KEY_FAV_DOWN },	//fb	
//	 { 0x82, KEY_FAV_UP },	//ff
	{ 0x98, KEY_FORWARD },
	{ 0x82, KEY_BACK },		
#if 0
	/*  0x1c   0x3   0x42  0x8  *
	 * POWER  AUDIO  SAT   MUTE *
	 *                          */
	{ 0x1c, KEY_POWER },
	{ 0x03, KEY_AUDIO },
	{ 0x42, KEY_SAT   },
	{ 0x08, KEY_MUTE  },

	/*  0x55    0x51       0x5e *
	 * ZOOM    TIMESHIFT   SUB  *
	 *                          */
	{ 0x55, KEY_ZOOM     },
	{ 0x51, KEY_TIME     },
	{ 0x5e, KEY_SUBTITLE },

	/*  0x5a    0x52     0x5d *
	 * TV/RADIO TTX   FILELIST  *
	 *                          */
	{ 0x5a, KEY_TV   },
	{ 0x52, KEY_TEXT },
	{ 0x5d, KEY_LIST },

	/*  0x18           0x17  *
	 * MENU           EXIT   *
	 *                       */
	{ 0x18, KEY_MENU },
	{ 0x17, KEY_EXIT },

	/*          0x1a          *
	 *           Up           *
	 *                        *
	 *  0x47    0x06    0x07  *
	 *  Left     Ok     Right *
	 *                        *
	 *         0x48           *
	 *         Down           *
	 *                        */
	{ 0x47, KEY_LEFT  },
	{ 0x1a, KEY_UP    },
	{ 0x07, KEY_RIGHT },
	{ 0x48, KEY_DOWN  },
	{ 0x06, KEY_OK },

	/*  0x49           0xa  *
	 * EPG           INFO   *
	 *                      */
	{ 0x49, KEY_EPG  },
	{ 0x0a, KEY_INFO },

	/*  0x54    0x16    0x15  *
	 *   1       2       3    *
	 *                        *
	 *  0x50    0x12    0x11  *
	 *   4       5       6    *
	 *                        *
	 *  0x4c    0xe    0xd    *
	 *   7       8       9    *
	 *                        */
	{ 0x54, KEY_NUMERIC_1 },
	{ 0x16, KEY_NUMERIC_2 },
	{ 0x15, KEY_NUMERIC_3 },
	{ 0x50, KEY_NUMERIC_4 },
	{ 0x12, KEY_NUMERIC_5 },
	{ 0x11, KEY_NUMERIC_6 },
	{ 0x4c, KEY_NUMERIC_7 },
	{ 0x0e, KEY_NUMERIC_8 },
	{ 0x0d, KEY_NUMERIC_9 },

	/*  0x10    0x41    0xc  *
	 * RECALL    FAV      0  *
	 *                       */
	{ 0x10, KEY_AGAIN     },
	{ 0x41, KEY_FAVORITES },
	{ 0x0c, KEY_NUMERIC_0 },


	/*  0x09       0x05        0x4b   0x4f *
	 *  LEFTSHIFT RIGHTSHIFT PREVIOUS NEXT *
	 *                                     */
	{ 0x09, KEY_LEFTSHIFT  },
	{ 0x05, KEY_RIGHTSHIFT },
	{ 0x4b, KEY_PREVIOUS   },
	{ 0x4f, KEY_NEXT       },

	/*  0x01  0x5f  0x19  0x58 *
	 *  PLAY PAUSE STOP RECORD *
	 *                         */
	{ 0x01, KEY_PLAY   },
	{ 0x5f, KEY_PAUSE  },
	{ 0x19, KEY_STOP   },
	{ 0x58, KEY_RECORD },

	/*  0x56  0x57  0x1f  0x5b *
	 *  RED  GREEN YELLO BLUE  *
	 *                         */
	{ 0x56, KEY_RED    },
	{ 0x57, KEY_GREEN  },
	{ 0x1f, KEY_YELLOW },
	{ 0x5b, KEY_BLUE   },

	/*  0x14              0x13      *
	 *  KEY_VOLUMEUP KEY_VOLUMEDOWN *
	 *                              */
	{ 0x14, KEY_VOLUMEUP    },
	{ 0x13, KEY_VOLUMEDOWN  },
#endif 	
};

static struct rc_map_list hcdemo_map = {
	.map = {
		.scan     = hcdemo,
		.size     = ARRAY_SIZE(hcdemo),
		.rc_proto = RC_PROTO_NEC,
		.name     = "rc-hcdemo",
	}
};

static int init_rc_map_hcdemo(void)
{
	return rc_map_register(&hcdemo_map);
}

module_system(rc_map_hcdemo, init_rc_map_hcdemo, NULL, 0)
