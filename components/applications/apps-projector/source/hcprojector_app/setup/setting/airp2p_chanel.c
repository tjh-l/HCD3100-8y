#include "app_config.h"
#include "screen.h"
#include "setup.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "com_api.h"
#include "osd_com.h"

#ifdef AIRP2P_SUPPORT
int airp2p_ch_vec[] = {STR_AIRP2P_CH_6, LINE_BREAK_STR, STR_AIRP2P_CH_44, LINE_BREAK_STR, \
    STR_AIRP2P_CH_149, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};
static int airp2p_ch_map[] = {6, 44, 149};

extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
extern lv_obj_t *new_widget_(lv_obj_t*, int title, int *,uint32_t index, int len, int w, int h);

int airp2p_ch_translate_to_index(int channel)
{
    int index = 0;
    
    switch(channel)
    {
        case 6:
            index = 0;
            break;
        case 44:
            index = 1;
            break;
        case 149:
            index = 2;
            break;
        default:
            index = 0;
            break;
    }

    return index;
}

static int airp2p_ch_set(int index)
{
    printf("set airp2p ch >> %d \n", index);
    projector_set_some_sys_param(P_AIRP2P_CH, airp2p_ch_map[index]);

    return 1;
}

static void airp2p_ch_btnsmatrix_event(lv_event_t *e)
{
    btnmatrix_event(e, airp2p_ch_set);
}

void airp2p_ch_widget(lv_obj_t *btn)
{
    lv_obj_t * obj = new_widget_(btn, STR_AIRP2P_CH, airp2p_ch_vec, \
        airp2p_ch_translate_to_index(projector_get_some_sys_param(P_AIRP2P_CH)), \
        HC_ARRAY_SIZE(airp2p_ch_vec),0,0);
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), airp2p_ch_btnsmatrix_event, LV_EVENT_ALL, btn);
}
#endif
