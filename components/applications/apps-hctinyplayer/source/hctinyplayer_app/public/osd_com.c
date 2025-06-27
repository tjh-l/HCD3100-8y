#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <hcuapi/input.h>
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include "key_event.h"


void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t depth, uint16_t item_count, uint16_t new_pos)
{
    if (new_pos > depth - 1)
    {
        list_ctrl->top = new_pos - (depth - 1);
        list_ctrl->cur_pos = new_pos;
        list_ctrl->new_pos = new_pos;
    }
    else
    {
        list_ctrl->top = 0;
        list_ctrl->cur_pos = new_pos;
        list_ctrl->new_pos = new_pos;
    }
    list_ctrl->depth = depth;
    list_ctrl->count = item_count;
}


bool osd_list_ctrl_shift(obj_list_ctrl_t *list_ctrl, int16_t shift, uint16_t *new_top, uint16_t *new_pos)
{
    int16_t point, top;
    uint16_t page_point;
    uint32_t page_moving;
    uint16_t check_cnt = 0;

    uint16_t shift_top;

    if (list_ctrl->count == 0 || shift == 0)
        return false;

    if (list_ctrl->new_pos < list_ctrl->top)
        list_ctrl->new_pos = list_ctrl->cur_pos = list_ctrl->top;
    if (list_ctrl->new_pos > list_ctrl->count)
        list_ctrl->top = list_ctrl->new_pos = list_ctrl->cur_pos = 0;

    page_point = list_ctrl->new_pos - list_ctrl->top;

    point = list_ctrl->new_pos;
    top   = list_ctrl->top;
    shift_top = list_ctrl->top;

    do
    {
        page_moving = (shift == list_ctrl->depth || shift == -list_ctrl->depth) ? 1 : 0;

        point += shift;

        /* If move out of current page, the top point also need to move.*/
        if ( (point < top) || (point >= (top + list_ctrl->depth) ) )
        {
            top += shift;
        }

        /* Moving in current page only.*/
        if (top == shift_top && point < list_ctrl->count)
        {
            // printf("%d: shift = %d,top=%d,point=%d\n", __LINE__, shift,top,point);
        }
        else if (top < 0)  /*Moving to unexist "up" page*/
        {
            printf("%d: top < 0\n", __LINE__);
            if (shift_top > 0) // Must be page moving
            {
                printf("%d: shift_top > 0\n", __LINE__);
                /* Need move to first page */
                //page_moving
                top = 0;

                if (page_moving)
                    point = page_point;
                else
                    top = point;

                printf("%d: page moving=%ld: top=%d, point = %d\n", __LINE__, page_moving, top, point);
            }
            else//shift_top == 0
            {
                /* Need move to last page */
                printf("%d: shift_top == 0\n", __LINE__);

                top = list_ctrl->count - list_ctrl->depth;
                if (top < 0)
                    top = 0;

                if (page_moving)
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;

                if (point >= list_ctrl->count)
                    point = list_ctrl->count - 1;
            }
        }
        else if (point >= list_ctrl->count)  /*Moving to unexist "down" page*/
        {
            printf("%d: point >= list_ctrl->count\n", __LINE__);

            if (shift_top + list_ctrl->depth < list_ctrl->count) // Must be page moving
            {
                //page_moving
                printf("%d: shift_top + list_ctrl->depth < list_ctrl->count\n", __LINE__);

                /* Need move to last page */
                top = list_ctrl->count - list_ctrl->depth;
                if (top < 0)
                    top = 0;

                if (page_moving)
                    point = top + page_point;
                else
                    point = list_ctrl->count - 1;
                if (point >= list_ctrl->count)
                    point = list_ctrl->count - 1;
            }
            else
            {
                /* Need move to first page */
                printf("%d: Need move to first page\n", __LINE__);

                //page_moving
                top = 0;

                if (page_moving)
                    point = page_point;
                else
                    point = 0;
            }
        }
        else
        {
            printf("?\n");
        }

        shift_top = top;
        check_cnt++;
    }
    while (0);

    list_ctrl->cur_pos = list_ctrl->new_pos = point;
    list_ctrl->top = top;

    *new_pos  = point;
    *new_top    = top;

    return true;
}

bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos)
{
    uint16_t old_top;
    uint16_t new_position = 0;
    uint16_t new_top;
    int16_t step = VKEY_NULL;
    bool ret;

    old_top = list_ctrl->top;

    switch (vkey)
    {
        case V_KEY_UP:
            step = -4;
            break;
        case V_KEY_DOWN:
            step = 4;
            break;
        case V_KEY_P_UP:
            step = -list_ctrl->depth;
            break;
        case V_KEY_P_DOWN:
            step = list_ctrl->depth;
            break;
        case V_KEY_LEFT:
            step = -1;
            break;
        case V_KEY_RIGHT:
            step = 1;
            break;
    }

    ret = osd_list_ctrl_shift(list_ctrl, step, &new_top, &new_position);
    *new_pos = new_position;
    if (!ret)
    {
        return false;
    }
    if (old_top != new_top)
    {
        return true;
    }
    return false;
}
