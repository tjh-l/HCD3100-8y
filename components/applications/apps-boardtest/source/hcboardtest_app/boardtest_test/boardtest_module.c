#include "boardtest_module.h"
#include "com_api.h"

static hc_boardtest_msg_t boardtest[BOARDTEST_NUM];


static const char *boardtest_error_msg[] =
{
    "open file failed",        /*BOARDTEST_ERROR_OPEN_FILE*/
    "close file failed",       /*BOARDTEST_ERROR_CLOSE_FILE*/
    "read file failed",        /*BOARDTEST_ERROR_READ_FILE*/
    "open device failed",      /*BOARDTEST_ERROR_OPEN_DEVICE*/
    "malloc memory failed",    /*BOARDTEST_ERROR_MOLLOC_MEMORY*/
    "free memory failed",      /*BOARDTEST_ERROR_FREE_MEMORY*/
    "play media file failed",  /*BOARDTEST_ERROR_MEDIA_FILE*/
    "ioctl device failed",     /*BOARDTEST_ERROR_IOCTL_DEVICE*/
    "create sys data failed",  /*BOARDTEST_ERROR_CREATE_DATA*/ 
    "store app data failed",   /*BOARDTEST_ERROR_STORE_DATA*/
    "wrong parameter data ",   /*BOARDTEST_ERROR_PRAR_DATA*/
    "check data wrong",        /*BOARDTEST_ERROR_CHECK_DATA*/
    "socket failed",           /*BOARDTEST_ERROR_SOCKET_DATA*/
};

const char *hc_boardtest_error_msg_get(int sort)
{
    return boardtest_error_msg[sort - BOARDTEST_ERROR_OPEN_FILE];
}

void hc_boardtest_module_register(hc_boardtest_msg_reg_t *test)
{
    boardtest[test->sort_name].boardtest_msg_reg = test;
    boardtest[test->sort_name].state = BOARDTEST_PENDING;
    boardtest[test->sort_name].isabled = BOARDTEST_DISABLE;
    boardtest[test->sort_name].msg_reg_flag = 1;
    boardtest[test->sort_name].detail = NULL;
}

hc_boardtest_msg_t *hc_boardtest_msg_get(int sort)
{
    return &boardtest[sort];
}

void create_boardtest_passfail_mbox(boardtest_module_e sort_name)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_PASSFAIL_MBOX_CREATE;
    ctl_msg.msg_code = sort_name;
    api_control_send_msg(&ctl_msg);

    boardtest_exit_control_send_msg(&ctl_msg);
}

void create_boardtest_ok_mbox(boardtest_module_e sort_name)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_OK_MBOX_CREATE;
    ctl_msg.msg_code = sort_name;
    api_control_send_msg(&ctl_msg);
}

void close_lvgl_osd(void)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_OSD_CLOSE;
    api_control_send_msg(&ctl_msg);
}

void open_lvgl_osd(void)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_OSD_OPEN;
    api_control_send_msg(&ctl_msg);
}

void write_boardtest_detail(boardtest_module_e sort_name, char *detail)
{
    if (boardtest[sort_name].detail)
    {
        free(boardtest[sort_name].detail);
        boardtest[sort_name].detail = NULL;
    }
    if (detail)
        boardtest[sort_name].detail = strdup(detail);
    /*need to free its memory after rsv message */
}

void write_boardtest_detail_instantly(boardtest_module_e sort_name, char *detail)
{
    if (boardtest[sort_name].detail)
    {
        free(boardtest[sort_name].detail);
        boardtest[sort_name].detail = NULL;
    }
    if (detail)
        boardtest[sort_name].detail = strdup(detail);
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_DISPLAY_DETAIL;
    ctl_msg.msg_code = sort_name;
    api_control_send_msg(&ctl_msg);
    /*need to free its memory after rsv message */
}

void hc_boardtest_module_all_register(void)
{
    for (int i = 0; i < BOARDTEST_NUM; i++)
    {
        if (boardtest[i].msg_reg_flag != 1)
        {
            boardtest[i].boardtest_msg_reg = malloc(sizeof(hc_boardtest_msg_reg_t));
            boardtest[i].state = BOARDTEST_PENDING;
            boardtest[i].isabled = BOARDTEST_DISABLE;
            boardtest[i].boardtest_msg_reg->english_name = NULL;
            // boardtest[i].boardtest_msg_reg->sort_name = NULL;
            boardtest[i].boardtest_msg_reg->init = NULL;
            boardtest[i].boardtest_msg_reg->run = NULL;
            boardtest[i].boardtest_msg_reg->exit = NULL;
            boardtest[i].boardtest_msg_reg->tips = NULL;
        }
    }
}
