#ifndef _MP_PREVIEW_H
#define _MP_PREVIEW_H

#define PREVIEW_W  640
#define PREVIEW_H  360
#define PREVIEW_X  640
#define PREVIEW_Y  360

typedef enum {
    PREVIEW_NONE=0,
    PREVIEW_NORMAL,
    PREVIEW_ERROR,
}preview_state_e;

extern lv_timer_t * preview_timer_handle;   /*timer handle */
extern lv_obj_t * ui_win_zoom;
extern lv_obj_t * ui_win_name;
extern lv_obj_t * ui_file_info;
extern bool is_preview_error;

int preview_init();
int preview_reset(void);
int preview_deinit(void);
void preview_timer_cb(lv_timer_t * t);
int preview_key_ctrl(uint32_t key);
void preview_player_message_ctrl(void * arg1,void *arg2);
preview_state_e win_preview_state_get(void);
void preview_player_message_ctrl(void * arg1,void *arg2);

#endif
