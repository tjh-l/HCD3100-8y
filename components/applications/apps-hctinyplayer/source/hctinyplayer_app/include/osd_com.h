#ifndef __OSD_COM_H__
#define __OSD_COM_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The structure is to adjust the position information of the list(LV widgets)
 */
typedef struct
{
    uint16_t top;        //In a page, the start index of file list
    uint16_t depth;      //In a page, the max count of list items
    uint16_t cur_pos;        //the current index of file list
    uint16_t new_pos;        //the new index of file list
    uint16_t select;        //the select index of file list
    uint16_t count; // all the count(dirs and files) of file list
} obj_list_ctrl_t;


void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t count, uint16_t item_count, uint16_t new_pos);
bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __OSD_COM_H__
