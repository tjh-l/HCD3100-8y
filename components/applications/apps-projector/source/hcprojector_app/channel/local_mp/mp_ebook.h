/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-10-24 21:10:47
 */
#ifndef __MP_EBOOK_H_
#define __MP_EBOOK_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef char			INT8;
typedef unsigned char	UINT8;

typedef short			INT16;
typedef unsigned short	UINT16;

typedef long			INT32;
typedef unsigned long	UINT32;

typedef unsigned long long UINT64;
typedef long long INT64;

typedef signed int INT;
typedef unsigned int UINT;

//extern bool is_ebook_bgmusic;

typedef struct
{
	UINT32 e_page_num;
	UINT32 e_page_byte;
	UINT32 e_page_seek;
}ebook_page_info_t;


void ebook_open(void);
//void ebook_close(void);
void ebook_keyinput_event_cb(lv_event_t *event);
void ebook_get_fullname(char *fullname, char *path, char *name);
void readtyped(FILE *fp_read);
UINT32  fgetws_ex(char *string, int n, FILE *fp);
void ebook_show_page_info(int save_flag);
void change_ebook_txt_info(int keypad_value,int set_page);
int ebook_read_file(char *ebook_file_name);
bool get_ebook_fp_state(void);
void win_ebook_close(void);
int ebook_read_file_task(char *name);
int ebook_read_file_pthread_stop(void);
static void ebook_free_data(void *data, void *user_data);
bool get_ebook_bgmusic_state(void);
void set_ebook_bgmusic_state(bool state);
void ebook_close(bool force);
#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif

