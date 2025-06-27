#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_fspage.h"
#include "mp_ebook.h"
#include "screen.h"
#include <hcuapi/mmz.h>
#include "gb_2312.h"
#include "mp_ctrlbarpage.h"
#include "mp_playlist.h"
#include "backstage_player.h"
//#include "mp_medialist.h"
#include "memmory_play.h"
#include "factory_setting.h"

#define EBOOK_FILE_SIZE_MAX	30*MAX_FILE_NAME*MAX_FILE_NAME	//30M ebook preview size
static uint32_t ebook_malloc_size = 2048;
#define DISPLAY_BUFF_SIZE	512
#define utf8_char_len(c) ((((int)0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1)
#define utf8_to_u_hostendian(str, uni_str, err_flag) \
{\
	err_flag = 0;\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		err_flag = 1;\
	}\
}


extern lv_group_t * ebook_group;
extern lv_obj_t * ui_ebook_label;
extern lv_obj_t * ui_ebook_label_page;
//extern lv_obj_t * lv_page_num;

typedef struct line_data{
	UINT32 line_number;
	UINT32 line_byte;
}line_data_t;

typedef enum FP_TYPE
{
	UTF8_TYPE=1,
	UTF16_BE_TYPE,	
	UTF16_LE_TYPE,
	UTF16_BE_TYPE_DIS_BOM,	
	UTF16_LE_TYPE_DIS_BOM,
	TYPE_NULL,
}FP_TYPE_T;

typedef struct
{
	FILE 	*fp_ebook;			//file descriptor
	bool 	is_ebook_bgmusic;	//Whether to turn on background music for ebooks
	bool 	check_utf8;			//Check for UTF-8 encoding
	UINT8  	cur_fp_type;		//Current ebook encoding format
	UINT16 	ebook_page_line;	//How many lines does an ebook display on a page
	UINT16 	ebook_line_max_bytes;//How many bytes are displayed in an ebook line
	UINT16 	save_line_num;		//Total number of lines in the file
	UINT32 	lseek_num;			//The number of bytes of the current page
	UINT32 	cur_page;			//Number of pages currently read
	UINT32	last_save_page;		//Number of pages read last time
	UINT32  half_byte_page;		//The number of pages required for the first half byte
	UINT32 	page_all;			//Total page count
	UINT32 	ebook_all_line;		//Total lines
	UINT32 	read_offset;		//The offset of the current page to 0
	long	file_size;			//Total file size
	long	file_ebook_size;	//The number of bytes remaining
	long	file_ebook_seek_size;//The number of bytes left on the previous page
}txt_ebook_data;

static line_data_t *ebook_line_info= NULL;//Each line of information
static UINT16 *get_buff_mul = NULL;//buff to show
static UINT16 *str_mul = NULL;//Temporary buff
static txt_ebook_data txt_data;
static txt_ebook_data txt_data_tmp={0};
lv_timer_t * bar_timer=NULL;//Ebook menu
static lv_timer_t * updata_page_timer=NULL;//When the thread reads the file, the total number of pages is updated periodically
/*The temporary memory allocation is used to preview part of the ebook content 
and quickly display the contents of large files. The maximum memory allocation 
is 512 KB
*/
static char *read_buff_tmp = NULL;
//When a single thread reads a file, 0 indicates that the file has been read and 1 indicates that the file is being read
static pthread_t read_ebook_thread_id = 0;//Thread id
static int pthread_read_ebook_state = -1;
static glist * ebook_list=NULL;//The content read by the thread is saved by page in the form of a linked list
static bool ebook_bgmusic_en=false;
// a var to record bgmusic state

static void ebook_info_save_cur_info_memmory(ebook_page_info_t *media_info_data);
static void ebook_info_save_memmory(char *name);

bool get_ebook_bgmusic_state(void)
{
	return ebook_bgmusic_en;
}

void set_ebook_bgmusic_state(bool state)
{
	ebook_bgmusic_en=state;
}

UINT32 ComAscStr2Uni(UINT8* Ascii_str,UINT16* Uni_str)
{
	UINT32 i=0;
	if((Ascii_str==NULL)||(Uni_str==NULL))
		return 0;
	while(Ascii_str[i])
	{
#if(SYS_CPU_ENDIAN==ENDIAN_LITTLE)
		Uni_str[i]=(UINT16)(Ascii_str[i]<<8);
#else
		Uni_str[i]=(UINT16)Ascii_str[i];
#endif
		i++;
	}

	Uni_str[i]=0;
	return i;
}
UINT16 *ComStr2UniStrExt(UINT16* uni, char* str, UINT16 maxcount)
{	
	UINT16 i;
	if (uni == NULL)
		return NULL;
	if (str == NULL || maxcount == 0)
	{
		uni[0]=(UINT16)'\0';
		return NULL;
	}
	for(i=0; (0!=str[i])&&(i<maxcount); i++)
#if (SYS_CPU_ENDIAN==ENDIAN_LITTLE)
		uni[i]=(UINT16)(str[i]<<8);
#else
		uni[i]=(UINT16)str[i];
#endif
	uni[i]=(UINT16)'\0';
	return uni;
}

#define u_hostendian_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = (UINT8)*uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|(*uni_str++&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|(*uni_str++&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}

bool IsUTF8(const void* pBuffer, long size)
{
	bool IsUTF8 = true;
	int error_time=0;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end = (unsigned char*)pBuffer + size;
	while (start < end)
	{
		if (*start < 0x80) // (10000000): 值小于0x80的为ASCII字符
		{
			start++;
		}
		else if (*start < (0xC0)) // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
		{
			error_time++;
			start++;
			if(error_time>3)
			{
				IsUTF8 = false;
				break;
			}
		}
		else if (*start < (0xE0)) // (11100000): 此范围内为2字节UTF-8字符
		{
			if (start >= end - 1)
			{
				break;
			}
			if ((start[1] & (0xC0)) != 0x80)
			{
				IsUTF8 = false;
				break;
			}
				start += 2;
		}
		else if (*start < (0xF0)) // (11110000): 此范围内为3字节UTF-8字符
		{
			if (start >= end - 2)
			{
				break;
			}
			if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
			{
				IsUTF8 = false;
				break;
			}
			start += 3;
		}
		else
		{
			IsUTF8 = false;
			break;
		}
	}
	return IsUTF8;
}

static int unicode_to_utf8(
	const UINT16 *src,
	int *srcLen,
	char *dst,
	int *dstLen)
{
	int result;
	int origlen = *srcLen;
	int srcLimit = *srcLen;
	int dstLimit = *dstLen;
	int srcCount = 0;
	int dstCount = 0;
	for (srcCount = 0; srcCount < srcLimit; srcCount++)
	{
		unsigned short  *UNICODE = (unsigned short *) & src[srcCount];
		unsigned char	utf8[4];
		unsigned char	*UTF8 = utf8;
		int utf8Len;
		int j;
		u_hostendian_to_utf8(UTF8, UNICODE);
		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}
	*srcLen = srcCount;
	*dstLen = dstCount;
	result = ((dstCount > 0) ? 0 : -1);
	if (*srcLen < origlen)
	{
		return -1;
	}
	return result;
}

UINT32 ComUniStrLen(const UINT16* string)
{
	UINT32 i=0;
	if(string == NULL)
		return 0;
	
	while (string[i])
		i++;
	return i;
}

INT32 ComUniStr2UTF8(UINT16* Uni_str, UINT8* utf8,unsigned long utf8len)
{
	INT32 result;
	unsigned long unilen;
	unilen = ComUniStrLen(Uni_str) + 1;
	result = unicode_to_utf8(Uni_str, (int *) & unilen, utf8, (int *) & utf8len);
	return result;
}

void ebook_get_fullname(char *fullname, char *path, char *name)
{
	strcpy(fullname, path);
	strcat(fullname, "/");
	strcat(fullname, name);
}

void ebook_free_buff(void)
{
	//txt_data.read_page_num = cur_page;
	if(updata_page_timer)
	{
		lv_timer_pause(updata_page_timer);
		lv_timer_del(updata_page_timer);
		updata_page_timer = NULL;
	}
	if(get_buff_mul!=NULL)
	{
		free(get_buff_mul);
		get_buff_mul = NULL;
	}
	if(str_mul!=NULL)
	{
		free(str_mul);
		str_mul = NULL;
	}
	if(ebook_line_info != NULL)
	{
		free(ebook_line_info);
		ebook_line_info = NULL;
	}
	fclose(txt_data.fp_ebook);
	txt_data.fp_ebook = NULL;
	txt_data.check_utf8=false;
	memset(&txt_data,0,sizeof(txt_ebook_data));
	memset(&txt_data_tmp,0,sizeof(txt_ebook_data));
	glist_free_full(ebook_list, ebook_free_data);
	ebook_list = NULL;
	
	txt_data.page_all = 1;
}
bool get_ebook_fp_state(void)
{
	if(txt_data.fp_ebook)
		return true;
	return false;
}

void win_ebook_close(void)
{
	if(NULL == txt_data.fp_ebook)
		return;
	if(read_buff_tmp)
	{
		free(read_buff_tmp);
		read_buff_tmp = NULL;
	}
	ebook_read_file_pthread_stop();
	//usleep(20*1000);
	ebook_free_buff();
}

void ebook_keyinput_page_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t * obj = lv_event_get_target(event);
	lv_obj_t * ta = lv_event_get_user_data(event); 
	const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
	sprintf(txt,"999");
	lv_textarea_add_text(ta, txt);
}

/*ebook page key ctrl in here
*  
*/
void ebook_keyinput_event_cb(lv_event_t *event)
{
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t * target=lv_event_get_target(event);
	if(code==LV_EVENT_PRESSED){
		lv_obj_clear_state(target,LV_STATE_PRESSED);
	}else if(code == LV_EVENT_KEY){
		uint32_t keypad_value = lv_indev_get_key(lv_indev_get_act());
		uint32_t vkey = key_convert2_vkey(keypad_value);
		switch (keypad_value){
			case LV_KEY_UP :            	
				if (m_play_bar_show==0)
					ctrlbar_btn_enter(ctrlbarbtn[0]);
				break;
			case LV_KEY_DOWN :
				if(m_play_bar_show==0)
					ctrlbar_btn_enter(ctrlbarbtn[1]);
				break;
			case LV_KEY_RIGHT :
				lv_group_focus_next(lv_group_get_default());
				break;
			case LV_KEY_LEFT :
					lv_group_focus_prev(lv_group_get_default());
				break;
			case LV_KEY_ESC :
				/*ctrlbar is on display */
				if(!lv_obj_has_flag(ui_play_bar,LV_OBJ_FLAG_HIDDEN)){
					show_play_bar(false);
				}else{
					_ui_screen_change(ui_fspage,0,0);
				}
				break;
			case LV_KEY_ENTER:
				/*ctlbar is on display*/
				if(!lv_obj_has_flag(ui_play_bar,LV_OBJ_FLAG_HIDDEN))
				{
					int ret = ctrlbar_btn_enter(target);
					if(ret == -1)
						return;
				}
				break;
			case LV_KEY_NEXT:
				ctrlbar_btn_enter(ctrlbarbtn[3]);
				break;
			case LV_KEY_PREV:
				ctrlbar_btn_enter(ctrlbarbtn[2]);
				break;
			default :
				break;
		}
		//show bar or not
		//barbtn4->stop  mean Esc ctrlpage
		if(target==ctrlbarbtn[4] && !lv_obj_has_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN))
			return ;
		else if(vkey == V_KEY_0 || vkey == V_KEY_1 || vkey == V_KEY_2 || vkey == V_KEY_3 ||\
			vkey == V_KEY_4 || vkey == V_KEY_5 || vkey == V_KEY_6 || vkey == V_KEY_7 || \
			vkey == V_KEY_8 || vkey == V_KEY_9)
		{
			vkey_other_btn(ui_ebook_txt,vkey,lv_group_get_default());
		}
		else if (keypad_value!=LV_KEY_ESC&&keypad_value!=LV_KEY_UP&&keypad_value!=LV_KEY_DOWN){   
			show_play_bar(true);
			if(bar_timer)
				lv_timer_reset(bar_timer);
		}else if(vkey==V_KEY_STOP){
			_ui_screen_change(ui_fspage,0,0);
		}
		if(m_play_bar_show == true)
		{
			if(bar_timer)
				lv_timer_reset(bar_timer);
		}
	}
}

/* Convert UTF-16 to UTF-8.  */
uint8_t *utf16_to_utf8_t(uint8_t *dest, const uint16_t *src, size_t size)
{
	uint32_t code_high = 0;
	while (size--) {
		uint32_t code = *src++;
		if (code_high) {
			if (code >= 0xDC00 && code <= 0xDFFF) {
				/* Surrogate pair.  */
				code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;
				*dest++ = (code >> 18) | 0xF0;
				*dest++ = ((code >> 12) & 0x3F) | 0x80;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			} else {
				/* Error...  */
				*dest++ = '?';
				/* *src may be valid. Don't eat it.  */
				src--;
			}
			code_high = 0;
		} else {
			if (code <= 0x007F) {
				*dest++ = code;
			} else if (code <= 0x07FF) {
				*dest++ = (code >> 6) | 0xC0;
				*dest++ = (code & 0x3F) | 0x80;
			} else if (code >= 0xD800 && code <= 0xDBFF) {
				code_high = code;
				continue;
			} else if (code >= 0xDC00 && code <= 0xDFFF) {
				/* Error... */
				*dest++ = '?';
			} else if (code < 0x10000) {
				*dest++ = (code >> 12) | 0xE0;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			} else {
				*dest++ = (code >> 18) | 0xF0;
				*dest++ = ((code >> 12) & 0x3F) | 0x80;
				*dest++ = ((code >> 6) & 0x3F) | 0x80;
				*dest++ = (code & 0x3F) | 0x80;
			}
		}
	}

	return dest;
}

UINT32 ComUniStrToMB(UINT16* pwStr)
{
	if(pwStr == NULL)
		return 0;
	UINT32 i=0;
	while(pwStr[i])
	{
		pwStr[i]=(UINT16)(((pwStr[i]&0x00ff)<<8) | ((pwStr[i]&0xff00)>>8));
		i++;
	}
	return i;
}

int ComUniStrCopyChar(UINT8 *dest, UINT8 *src)
{	
	unsigned int i;
	if((NULL == dest) || (NULL == src))
		return 0;
	for(i=0; !((src[i] == 0x0 && src[i+1] == 0x0)&&(i%2 == 0)) ;i++)
		dest[i] = src[i];
	if(i%2)
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = dest[i+1] = 0x0;
	return i/2;
}

void readtyped(FILE *fp_read)
{
	unsigned char buff[MAX_FILE_NAME];
	int ret=0;
	if(fp_read == NULL)
	{
		return;
	}
	memset(buff, 0, MAX_FILE_NAME);
	//fgets(buff,MAX_FILE_NAME,fp_read);
	int i = 0;
	while(i < MAX_FILE_NAME && !feof(fp_read))
	{
		fread(&buff[i], sizeof(char), 1, fp_read);
		if (i == 0)
		{
			i ++;
			continue;
		}
		if(
			(buff[i-1] == 0x0a && buff[i] == 0x0) ||
			(buff[i-1] == '\0' && buff[i] =='\n') ||
			(buff[i-1] == 0x0d && buff[i] == 0x0a) ||
			(buff[i-1] == 0x00 && buff[i] == 0x0a) ||
			(buff[i] == 0x0a)
			)
		{
			break;
		}
		i++;
	}
	if (i >= MAX_FILE_NAME)
		i = MAX_FILE_NAME-1;
	if((i%2) == 0 && !(buff[i-1] == 0x0d && buff[i]==0x0a))
	{
		i++;
		if (i >= MAX_FILE_NAME)
			i = MAX_FILE_NAME-1;
	}
	txt_data.ebook_page_line = 15;
	txt_data.ebook_line_max_bytes = 80;
	if(buff[0] == 0xef && buff[1] == 0xbb)
		txt_data.cur_fp_type =  UTF8_TYPE;
	else if(buff[0] == 0xff && buff[1] == 0xfe)
	{
		txt_data.cur_fp_type =  UTF16_LE_TYPE;
	}
	else if(buff[0] == 0xfe && buff[1] == 0xff)
	{
		txt_data.cur_fp_type =	UTF16_BE_TYPE;
	}
	else if((buff[i-3] == 0x0d && buff[i] == 0x0a))
	{
		txt_data.cur_fp_type =  UTF16_LE_TYPE_DIS_BOM;
	}
	else if((buff[i-1] == 0x00 && buff[i] != 0x00))
	{
		txt_data.cur_fp_type =	UTF16_BE_TYPE_DIS_BOM;
	}
	else
		txt_data.cur_fp_type = TYPE_NULL;
}

UINT32  fgetws_ex(char *string, int n, FILE *fp)
{
	if(!string || !fp || feof(fp) || n <=0)
	{
		return 0;
	}
	int i = 0;
	while(i < n-1 && !feof(fp))
	{
		fread(&string[i], sizeof(UINT16), 1, fp);
		if((string[i-2] == 0x0d &&string[i-1]==0x0&&string[i] == 0x0a &&string[i+1]==0x0) || (string[i] == '\0' && string[i+1]=='\n'))
		{
			i+=2;
			break;
		}
		i+=2;
	}
	string[i] = 0x0;
	return i;//string;
}

void ebook_show_page_info(int save_flag)
{
	char page_info[64]={0};
	UINT32 cur_page_tmp=0;
	ebook_page_info_t *ebook_page_info = NULL;
	if(pthread_read_ebook_state == 1)
	{
		if(ebook_list && save_flag == 1)
		{
			ebook_page_info = ebook_list->data;
			ebook_page_info->e_page_num = txt_data.cur_page;
			ebook_info_save_cur_info_memmory(ebook_page_info);
		}
		sprintf(page_info,"%ld / %ld",txt_data.cur_page,txt_data.page_all);
	}
	else
	{
		if(txt_data.read_offset >= ((DISPLAY_BUFF_SIZE/2) *MAX_FILE_NAME))
		{
			cur_page_tmp = txt_data.cur_page+(txt_data.last_save_page-txt_data.save_line_num/txt_data.ebook_page_line);
			sprintf(page_info,"%ld / %ld",txt_data.cur_page+(txt_data.last_save_page-txt_data.save_line_num/txt_data.ebook_page_line),txt_data.page_all+(txt_data.last_save_page-txt_data.save_line_num/txt_data.ebook_page_line));
		}
		else
		{
			cur_page_tmp = txt_data.cur_page;
			sprintf(page_info,"%ld / %ld",txt_data.cur_page,txt_data.page_all);
		}
		if(save_flag == 1)
		{
			ebook_page_info = malloc(sizeof(ebook_page_info_t));
			memset(ebook_page_info, 0 , sizeof(ebook_page_info_t));
			ebook_page_info->e_page_num = cur_page_tmp;
			if(ebook_line_info)
			{
				ebook_page_info->e_page_byte= ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte-
											  ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)].line_byte;
				ebook_page_info->e_page_seek= ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
			}
			ebook_info_save_cur_info_memmory(ebook_page_info);
			free(ebook_page_info);
			ebook_page_info = NULL;
		}
	}
	lv_label_set_text(ui_ebook_label_page,page_info);
}

void change_ebook_txt_info(int keypad_value,int set_page)
{
	UINT32 lseek_num_tmp=0;
	int i;
	ebook_page_info_t *ebook_page_info = NULL;
	int offset_page=0;
	if(keypad_value == LV_KEY_DOWN)
	{
		if(txt_data.cur_page == txt_data.page_all || txt_data.cur_page == 0)
			return;
		lseek_num_tmp = 0;
		txt_data.cur_page++;
		if(pthread_read_ebook_state == 1)
		{
			ebook_list = glist_nth((glist *)ebook_list,1);
			ebook_page_info = ebook_list->data;
			lseek_num_tmp = ebook_page_info->e_page_byte;
			if(lseek_num_tmp > 0 && (lseek_num_tmp % 2) > 0)
				lseek_num_tmp += 1;

			txt_data.lseek_num = lseek_num_tmp;
			if((ebook_page_info->e_page_seek-ebook_page_info->e_page_byte) > 0 && ((ebook_page_info->e_page_seek-ebook_page_info->e_page_byte) % 2) > 0)
				fseek(txt_data.fp_ebook,ebook_page_info->e_page_seek-ebook_page_info->e_page_byte-1,SEEK_SET);
			else
				fseek(txt_data.fp_ebook,ebook_page_info->e_page_seek-ebook_page_info->e_page_byte,SEEK_SET);
		}
		else
		{
			if(txt_data.cur_page == txt_data.page_all)
			{
				lseek_num_tmp = txt_data.file_ebook_size;
				txt_data.lseek_num = lseek_num_tmp;
			}
			else
			{
				lseek_num_tmp = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
				txt_data.lseek_num = lseek_num_tmp;
			}
		}
	}
	else if(keypad_value == LV_KEY_UP)
	{
		if(txt_data.cur_page==1 || txt_data.cur_page == 0)
		{
			return;
		}
		txt_data.cur_page--;
		lseek_num_tmp = 0;
		if(pthread_read_ebook_state == 1)
		{
			ebook_list = glist_nth_prev((glist *)ebook_list,1);
			ebook_page_info = ebook_list->data;
			if(txt_data.cur_page == 1)
				lseek_num_tmp = ebook_page_info->e_page_byte;
			else
				lseek_num_tmp = ebook_page_info->e_page_byte;
			txt_data.file_ebook_size = txt_data.file_ebook_seek_size+lseek_num_tmp;
			if(lseek_num_tmp > 0 && (lseek_num_tmp % 2) > 0)
				lseek_num_tmp += 1;
			
			txt_data.lseek_num = lseek_num_tmp;
			if((ebook_page_info->e_page_seek-ebook_page_info->e_page_byte) > 0 && ((ebook_page_info->e_page_seek-ebook_page_info->e_page_byte) % 2) > 0)
				fseek(txt_data.fp_ebook,ebook_page_info->e_page_seek-ebook_page_info->e_page_byte-1,SEEK_SET);
			else
				fseek(txt_data.fp_ebook,ebook_page_info->e_page_seek-ebook_page_info->e_page_byte,SEEK_SET);

			if(txt_data.cur_page == 1)
			{
				if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
					fseek(txt_data.fp_ebook,2,SEEK_SET);
			}
		}
		else
		{
			if(txt_data.cur_page == 1)
				lseek_num_tmp = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
			else
				lseek_num_tmp = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte - ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
			txt_data.lseek_num = lseek_num_tmp;
			txt_data.file_ebook_size = txt_data.file_ebook_seek_size+lseek_num_tmp;
		}
	}
	else
	{
		if(pthread_read_ebook_state == 0 || set_page <= 0 || set_page > txt_data.page_all || set_page == txt_data.cur_page)
		{
			return;
		}
		if(txt_data.cur_page > set_page)
		{
			offset_page = txt_data.cur_page - set_page;
			ebook_list = glist_nth_prev((glist *)ebook_list,offset_page);
		}
		else if(txt_data.cur_page < set_page)
		{
			offset_page = set_page - txt_data.cur_page;
			ebook_list = glist_nth((glist *)ebook_list,offset_page);
		}
		
		txt_data.cur_page = set_page;
		//ebook_list = glist_nth((glist *)ebook_list,1);
		ebook_page_info = ebook_list->data;
		lseek_num_tmp = ebook_page_info->e_page_byte;
		txt_data.lseek_num = lseek_num_tmp;
		fseek(txt_data.fp_ebook,ebook_page_info->e_page_seek-ebook_page_info->e_page_byte,SEEK_SET);
	}

	if (txt_data.lseek_num * 2 > ebook_malloc_size)
	{
		if(str_mul!=NULL){
			free(str_mul);
			str_mul=NULL;
			ebook_malloc_size=txt_data.lseek_num*2;
			str_mul = (UINT16 *)malloc(ebook_malloc_size*2);
			if(str_mul == NULL)
			{
				printf("str_mul malloc faild\n");
				return;
			}
		}
		if(get_buff_mul!=NULL)
		{
			free(get_buff_mul);
			get_buff_mul=NULL;
			ebook_malloc_size=txt_data.lseek_num*2;
			get_buff_mul = (UINT16 *)malloc(ebook_malloc_size);
			if(get_buff_mul == NULL)
			{
				printf("get_buff_mul malloc faild\n");
				return;
			}
		}
	}

	memset(str_mul, 0, ebook_malloc_size*2);
	memset(get_buff_mul, 0, ebook_malloc_size);
	if(pthread_read_ebook_state == 1)
	{
		fread((UINT8 *)get_buff_mul,txt_data.lseek_num,1,txt_data.fp_ebook);
	}
	else
	{
		if(txt_data.cur_page == 1)
		{
			if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			{
				//fseek(txt_data.fp_ebook,2,SEEK_SET);
				memcpy((UINT8 *)get_buff_mul,read_buff_tmp+(txt_data.file_size-txt_data.file_ebook_size)*sizeof(char)+2*sizeof(char),\
						txt_data.lseek_num*sizeof(char));
			}
			else
			{
				memcpy((UINT8 *)get_buff_mul,read_buff_tmp+(txt_data.file_size-txt_data.file_ebook_size)*sizeof(char),\
						txt_data.lseek_num*sizeof(char));
			}
		}
		else
		{
			memcpy((UINT8 *)get_buff_mul,read_buff_tmp+(txt_data.file_size-txt_data.file_ebook_size)*sizeof(char),\
					txt_data.lseek_num*sizeof(char));
		}
	}
	txt_data.file_ebook_seek_size = txt_data.file_ebook_size;
	txt_data.file_ebook_size -= txt_data.lseek_num;
	if(txt_data.cur_fp_type ==  UTF8_TYPE)
	{
		printf("UTF8\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE_DIS_BOM)
	{
		printf("feff\n");
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,txt_data.lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(txt_data.cur_fp_type ==  UTF16_BE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE_DIS_BOM)
	{
		printf("fffe\n");
		ComUniStrToMB(get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,txt_data.lseek_num);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		if(txt_data.check_utf8)
		{
			lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
		}
		else
		{
			convert_gb2312_to_unicode((UINT8 *)get_buff_mul,lseek_num_tmp,str_mul,ebook_malloc_size);
			ComUniStrToMB((UINT16 *)str_mul);
			memset(get_buff_mul,0,ebook_malloc_size);
			ComUniStrCopyChar(  (UINT8 *)get_buff_mul,(UINT8 *)str_mul);
			memset(str_mul,0,ebook_malloc_size*2);
			utf16_to_utf8_t((UINT8 *)str_mul,get_buff_mul,lseek_num_tmp);
			lv_label_set_text(ui_ebook_label,(char *)str_mul);
		}
	}
	ebook_show_page_info(1);
}

static void ebook_info_save_cur_info_memmory(ebook_page_info_t *media_info_data)
{
	if(m_cur_file_list != NULL && (m_cur_file_list->file_count > 0))
	{
		file_node_t *file_node = NULL;
		char play_path_name[1024];
		file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
		ebook_get_fullname(play_path_name,m_cur_file_list->dir_path,file_node->name);

		sys_data_set_ebook_offset(play_path_name, media_info_data);
	}
#ifdef HC_MEDIA_MEMMORY_PLAY    
	memmory_play_set_state(0);
#endif    
}

static void ebook_info_save_memmory(char * name)
{
	play_info_t media_info_data={0};
	media_info_data.current_time = 0;
	memcpy(media_info_data.path,name,strlen(name)+1);
	media_info_data.type = MEDIA_TYPE_TXT;
	media_info_data.current_offset = 0;
	media_info_data.current_time= 0;
	media_info_data.last_page=0;

	sys_data_set_media_info(&media_info_data);
}

int ebook_read_file(char *ebook_file_name)
{
	struct stat  e_sa;
	char read_line[MAX_FILE_NAME] = {0};
	file_node_t *file_node = NULL;
	UINT32 read_tmp=0;
	UINT32 read_size_tmp = 0;
	int i=0;
	UINT32 page_size=0;
	UINT16 line_size = 0;
	save_node_t save_node={0};
	/*1、free memory */
	win_ebook_close();
	UINT8 offset_tmp=0;
	memset(read_line,0,MAX_FILE_NAME);
	if(ebook_file_name)
	{
		strncpy(read_line,ebook_file_name,MAX_FILE_NAME);
	}
	else
	{
		file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
		ebook_get_fullname(read_line,m_cur_file_list->dir_path,file_node->name);
	}

#ifdef HC_MEDIA_MEMMORY_PLAY 
	ebook_info_save_memmory(ebook_file_name);
#endif

	/*2、open file */
	txt_data.fp_ebook = fopen(read_line,"r");
	if(txt_data.fp_ebook == NULL)
	{
		printf("%s,%d,open %s file failed\n",__func__,__LINE__,read_line);
		return -1;
	}
	if(stat(read_line,&e_sa) == -1)
	{
		printf("stat failed\n");
		return -1;
	}
	if(e_sa.st_size > EBOOK_FILE_SIZE_MAX)
		return -1;
	if(get_buff_mul == NULL ||str_mul == NULL)
	{
		get_buff_mul =	(UINT16  *)malloc(ebook_malloc_size);
		str_mul =	(UINT16 *)malloc(ebook_malloc_size*2);
	}
	memset(str_mul, 0, ebook_malloc_size*2);
	memset(get_buff_mul, 0, ebook_malloc_size);
	txt_data.cur_page = 1;
	txt_data.lseek_num = 0;
	txt_data.file_ebook_seek_size = 0;
	txt_data.file_ebook_size = 0;
	txt_data.half_byte_page = 0;
	fseek(txt_data.fp_ebook,0,SEEK_SET);
	readtyped(txt_data.fp_ebook);
	txt_data.file_ebook_size = (long)e_sa.st_size;
	txt_data.file_size=txt_data.file_ebook_size;
	txt_data.file_ebook_seek_size = txt_data.file_ebook_size;
	
	/*
	If the file size is larger than 512k, save only 256k of the last memorized 
	location to a temporary buff, display the buff's contents first, and then 
	create a new thread to iterate through the entire article.
	*/
	if((save_node.save_offset-save_node.save_time > 0 && txt_data.file_ebook_size < DISPLAY_BUFF_SIZE*MAX_FILE_NAME) || 
		(save_node.save_offset-save_node.save_time > 0 && save_node.save_offset-save_node.save_time < ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))
 	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
		txt_data.read_offset = save_node.save_offset-save_node.save_time;
 	}
	else if(save_node.save_offset-save_node.save_time > ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))+2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))),SEEK_SET);
		txt_data.read_offset = (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME;
	}
	else
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
	}
	
	if(txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE || 
		txt_data.cur_fp_type ==  UTF16_LE_TYPE_DIS_BOM|| txt_data.cur_fp_type ==  UTF16_BE_TYPE_DIS_BOM)
	{
		while((line_size = fgetws_ex(read_line,MAX_FILE_NAME,txt_data.fp_ebook)) >0)
		{
			read_size_tmp += line_size;
			if(read_size_tmp > DISPLAY_BUFF_SIZE *MAX_FILE_NAME)
				break;
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				read_tmp += line_size/txt_data.ebook_line_max_bytes;
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					read_tmp ++;	
				}
			}
			else
			{
				read_tmp++;
			}
			memset(read_line,0,MAX_FILE_NAME);
		}
	}
	else
	{
		while(fgets(read_line,MAX_FILE_NAME,txt_data.fp_ebook))
		{
			line_size = strlen(read_line);
			read_size_tmp += line_size;
			if(txt_data.half_byte_page == 0 && read_size_tmp > (DISPLAY_BUFF_SIZE/2) *MAX_FILE_NAME)
				txt_data.half_byte_page = read_tmp/txt_data.ebook_page_line;
			
			if(read_size_tmp > DISPLAY_BUFF_SIZE *MAX_FILE_NAME)
				break;
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				read_tmp += line_size/txt_data.ebook_line_max_bytes;
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					read_tmp ++;
				}
			}
			else
			{
				read_tmp++;
			}
			memset(read_line,0,sizeof(read_line));
		}
	}
	if(read_tmp == 0)
	{
		lv_label_set_text(ui_ebook_label,"");
		txt_data.file_ebook_seek_size = 0;
		txt_data.file_ebook_seek_size = 0;
		txt_data.read_offset = 0;
		txt_data.lseek_num = 0;
		txt_data.save_line_num = 0;
		txt_data.ebook_all_line = 0;
		txt_data.cur_page = 0;
		txt_data.page_all = 0;
		goto empty_file;
	}
	ebook_line_info = malloc((read_tmp+1)*sizeof(line_data_t));
	memset(read_line,0,sizeof(read_line));
	read_tmp = 0;
	read_size_tmp = 0;
	if((save_node.save_offset-save_node.save_time > 0 && txt_data.file_ebook_size < DISPLAY_BUFF_SIZE*MAX_FILE_NAME) || 
		(save_node.save_offset-save_node.save_time > 0 && save_node.save_offset-save_node.save_time < ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))
 	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
		txt_data.read_offset = save_node.save_offset-save_node.save_time;
 	}
	else if(save_node.save_offset-save_node.save_time > ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))+2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))),SEEK_SET);
		txt_data.read_offset = (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME;
	}
	else
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
	}
	
	if(txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE ||
		txt_data.cur_fp_type ==  UTF16_LE_TYPE_DIS_BOM|| txt_data.cur_fp_type ==  UTF16_BE_TYPE_DIS_BOM)
	{
		while((line_size = fgetws_ex(read_line,MAX_FILE_NAME,txt_data.fp_ebook)) >0)
		{
			read_size_tmp += line_size;
			if(read_size_tmp > DISPLAY_BUFF_SIZE *MAX_FILE_NAME)
			{
				read_size_tmp -= line_size;
				break;
			}
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				for(i=0;i<line_size/txt_data.ebook_line_max_bytes;i++)
				{
					ebook_line_info[read_tmp+i].line_number = read_tmp+i;
					txt_data.lseek_num += txt_data.ebook_line_max_bytes;
					ebook_line_info[read_tmp+i].line_byte = txt_data.lseek_num;
				}
				read_tmp += line_size/txt_data.ebook_line_max_bytes;
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					ebook_line_info[read_tmp].line_number = read_tmp;
					txt_data.lseek_num += line_size%txt_data.ebook_line_max_bytes;
					ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
					read_tmp ++;	
				}
			}
			else
			{
				ebook_line_info[read_tmp].line_number = read_tmp;
				txt_data.lseek_num += line_size;
				ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
				read_tmp++;
			}
			if(txt_data.read_offset == (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME && read_size_tmp >= (txt_data.read_offset/2) && offset_tmp == 0)
			{
				offset_tmp = 1;
				txt_data.save_line_num = read_tmp;
			}
			else if(txt_data.read_offset <= (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME && read_size_tmp >= (txt_data.read_offset) && offset_tmp == 0)
			{
				offset_tmp = 1;
				txt_data.save_line_num = read_tmp;
			}
			memset(read_line,0,MAX_FILE_NAME);
		}
	}
	else
	{
		while(fgets(read_line,MAX_FILE_NAME,txt_data.fp_ebook))
		{
			line_size = strlen(read_line);
			read_size_tmp += line_size;
			if(read_size_tmp > DISPLAY_BUFF_SIZE *MAX_FILE_NAME)
			{
				read_size_tmp -= line_size;
				break;
			}
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				for(i=0;i<line_size/txt_data.ebook_line_max_bytes;i++)
				{
					ebook_line_info[read_tmp+i].line_number = read_tmp+i;
					txt_data.lseek_num += txt_data.ebook_line_max_bytes;
					ebook_line_info[read_tmp+i].line_byte = txt_data.lseek_num;
				}
				read_tmp += line_size/txt_data.ebook_line_max_bytes;
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					ebook_line_info[read_tmp].line_number = read_tmp;
					txt_data.lseek_num += line_size%txt_data.ebook_line_max_bytes;
					ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
					read_tmp ++;
					
				}
			}
			else
			{
				ebook_line_info[read_tmp].line_number = read_tmp;
				txt_data.lseek_num += line_size;
				ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
				read_tmp++;
			}
			if(txt_data.read_offset == (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME && read_size_tmp >= (txt_data.read_offset/2) && offset_tmp == 0)
			{
				offset_tmp = 1;
				txt_data.save_line_num = read_tmp;
			}
			else if(txt_data.read_offset <= (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME && read_size_tmp >= (txt_data.read_offset) && offset_tmp == 0)
			{
				offset_tmp = 1;
				txt_data.save_line_num = read_tmp;
			}
			memset(read_line,0,sizeof(read_line));
		}
	}
	if((save_node.save_offset-save_node.save_time > 0 && txt_data.file_ebook_size < DISPLAY_BUFF_SIZE*MAX_FILE_NAME) || 
		(save_node.save_offset-save_node.save_time > 0 && save_node.save_offset-save_node.save_time < ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))
 	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
		txt_data.read_offset = save_node.save_offset-save_node.save_time;
 	}
	else if(save_node.save_offset-save_node.save_time > ((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME)))+2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,(save_node.save_offset-(save_node.save_time+((DISPLAY_BUFF_SIZE/2) * MAX_FILE_NAME))),SEEK_SET);
		txt_data.read_offset = (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME;
	}
	else
	{
		if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
			fseek(txt_data.fp_ebook,2,SEEK_SET);
		else 
			fseek(txt_data.fp_ebook,0,SEEK_SET);
	}
	read_buff_tmp  = malloc(read_size_tmp);
	fread(read_buff_tmp,1,read_size_tmp,txt_data.fp_ebook);
	txt_data.ebook_all_line = read_tmp;
	txt_data.cur_page = txt_data.save_line_num/txt_data.ebook_page_line + 1;
	txt_data.page_all = read_tmp/txt_data.ebook_page_line;
	if(read_tmp % txt_data.ebook_page_line)
		txt_data.page_all++;
	if(txt_data.cur_fp_type ==  UTF8_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE)
		fseek(txt_data.fp_ebook,2,SEEK_SET);
	else 
		fseek(txt_data.fp_ebook,0,SEEK_SET);
	if(txt_data.cur_page == 1)
	{
		if(read_tmp < txt_data.ebook_page_line && read_tmp > 0)
			page_size = ebook_line_info[read_tmp-1].line_byte;
		else
			page_size = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
	}
	else
	{
		if(txt_data.cur_page == txt_data.page_all)
			page_size = ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
		else
			page_size = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
	}
	if(page_size>ebook_malloc_size){
		ebook_malloc_size=page_size;
		if(str_mul!=NULL){
			str_mul =	(UINT16 *)realloc(str_mul,ebook_malloc_size*2);		
			if(str_mul == NULL)
			{
				printf("str_mul realloc faild\n");
				return -1;
			}
		}
		if(get_buff_mul!=NULL)
		{
			get_buff_mul =	(UINT16  *)realloc(get_buff_mul,ebook_malloc_size);
			if(get_buff_mul == NULL)
			{
				printf("get_buff_mul realloc faild\n");
				return -1;
			}
		}
	}
	
	if(txt_data.cur_page == 1)
	{
		if(read_tmp < txt_data.ebook_page_line && read_tmp > 0)
		{
			memcpy((UINT8 *)get_buff_mul,read_buff_tmp,ebook_line_info[read_tmp-1].line_byte);
			page_size = ebook_line_info[read_tmp-1].line_byte;
			txt_data.file_ebook_size -= ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			memcpy((UINT8 *)get_buff_mul,read_buff_tmp,ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte);
			page_size = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
			txt_data.file_ebook_size-=ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
		}
	}
	else
	{
		if(txt_data.cur_page == txt_data.page_all)
		{
			memcpy((UINT8 *)get_buff_mul,read_buff_tmp+ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte,\
					ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte);
			page_size = ebook_line_info[read_tmp-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
			txt_data.file_ebook_size-=ebook_line_info[read_tmp-1].line_byte;
		}
		else
		{
			memcpy((UINT8 *)get_buff_mul,read_buff_tmp+ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte,\
					ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte);
			page_size = ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte-ebook_line_info[((txt_data.cur_page-1)*txt_data.ebook_page_line)-1].line_byte;
			txt_data.file_ebook_size-=ebook_line_info[((txt_data.cur_page)*txt_data.ebook_page_line)-1].line_byte;
		}
	}
	
	if(txt_data.cur_fp_type ==  UTF8_TYPE)
	{
		printf("UTF8\n");
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_LE_TYPE_DIS_BOM)
	{
		printf("feff\n");
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,page_size);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else if(txt_data.cur_fp_type ==  UTF16_BE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE_DIS_BOM)
	{
		printf("fffe\n");
		ComUniStrToMB((UINT16 *)get_buff_mul);
		ComUniStrCopyChar( (UINT8 *)str_mul, (UINT8 *)get_buff_mul);
		memset(get_buff_mul,0,ebook_malloc_size);
		utf16_to_utf8_t((UINT8 *)get_buff_mul,str_mul,page_size);
		lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
	}
	else
	{
		printf("other\n");
		
		if(IsUTF8(get_buff_mul,page_size))
		{
			txt_data.check_utf8 = true;
			lv_label_set_text(ui_ebook_label,(char *)get_buff_mul);
		}
		else
		{
			convert_gb2312_to_unicode((UINT8 *)get_buff_mul,page_size,str_mul,ebook_malloc_size);
			ComUniStrToMB((UINT16 *)str_mul);
			memset(get_buff_mul,0,ebook_malloc_size);
			ComUniStrCopyChar(  (UINT8 *)get_buff_mul,(UINT8 *)str_mul);
			memset(str_mul,0,ebook_malloc_size*2);
			utf16_to_utf8_t((UINT8 *)str_mul,get_buff_mul,page_size);
			lv_label_set_text(ui_ebook_label,(char *)str_mul);
		}
	}
	read_tmp=0;
	txt_data.file_ebook_seek_size = txt_data.file_ebook_size;
	txt_data.file_ebook_seek_size +=	page_size;
empty_file:
	ebook_show_page_info(1);
	ebook_read_file_task(ebook_file_name);
	return 0;
}
void ebook_updata_page_timer_cb(lv_timer_t * t)
{
	if(pthread_read_ebook_state == 0)
	{
		if(txt_data_tmp.page_all > txt_data.page_all)
			txt_data.page_all = txt_data_tmp.page_all;
	}
	else if(pthread_read_ebook_state == 1)
	{
		lv_timer_pause(updata_page_timer);
	}
	ebook_show_page_info(0);
}

void ebook_open(void)
{
	char ebook_file_name[MAX_FILE_NAME] = {0};
	char *token;
	char *token_tmp=NULL;
	save_node_t save_node = {0};
	if(last_scr==SCREEN_SETUP||last_scr==SCREEN_CHANNEL)
	{
		//change screen from setup or channel, do not need
		//create object again
		if (ebook_group)
			set_key_group(ebook_group);
		show_play_bar(true);
		return;
	}
	ebook_group= lv_group_create();
	ebook_group->auto_focus_dis = 1;
	set_key_group(ebook_group);
	create_ebook_scr(ui_ebook_txt,ebook_keyinput_event_cb);
	memset(ebook_file_name,0,MAX_FILE_NAME);
	file_node_t *file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
#ifdef HC_MEDIA_MEMMORY_PLAY
	if((1==mp_get_auto_play_state()) && 0<=(auto_playing_from_disk(m_cur_file_list,ebook_file_name, \
		&save_node,m_cur_file_list->media_type)))
	{
		mp_set_auto_play_state(0);
		strncpy(ebook_file_name,save_node.save_name,MAX_FILE_NAME);
		token = strtok(save_node.save_name,"/");
		while(token != NULL)
		{
			token = strtok(NULL,"/");
			if(token != NULL)
			{
				token_tmp = token;
			}
		}
		if(token_tmp != NULL)
		{
			m_cur_file_list->item_index = file_mgr_get_index_by_file_name(m_cur_file_list, token_tmp);
			file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
			txt_data.last_save_page = save_node.idx-1;
			strcpy(file_node->name,token_tmp);
		}
	}
	else if(get_cur_playing_from_disk(m_cur_file_list,ebook_file_name,&save_node,m_cur_file_list->media_type)>=0)
	{
		txt_data.last_save_page = save_node.idx-1;
	}
	else
	{
		ebook_info_save_memmory(ebook_file_name);
	}

	memmory_play_set_state(0);
	
#endif 
	ebook_get_fullname(ebook_file_name,m_cur_file_list->dir_path,file_node->name);
	int ret = ebook_read_file(ebook_file_name);	//start to read file
	if(ret < 0)
	{
		control_msg_t objfresh_msg;
		win_msgbox_msg_open_on_top(STR_FILE_FAIL, 2000, NULL, NULL);
		objfresh_msg.msg_type =  MSG_TYPE_REMOTE;
		objfresh_msg.msg_code = MSG_TYPE_BAR_NO_FILE_MSG;
		api_control_send_msg(&objfresh_msg);
		//return;
	}
	else
	{
		lv_label_set_text(ui_playname,file_node->name);
		playlist_init();
		bar_timer=lv_timer_create(bar_show_timer_cb, 5000, NULL);
		app_set_screen_submp(SCREEN_SUBMP3);
	}
}

static void ebook_free_data(void *data, void *user_data)
{
	(void)user_data;
	if (data)
		free(data);
}

extern SCREEN_TYPE_E last_scr;
extern SCREEN_TYPE_E cur_scr;
void ebook_close(bool force)
{
	lv_obj_t *obj = lv_scr_act();
	if (!force){
		//change screen to setup or channel, do not need
		//delete objects. playing background.
		if(cur_scr==SCREEN_CHANNEL||cur_scr==SCREEN_SETUP)//perss EPG /MENU
		{
			return;
		}
	}
	if (!ebook_group)
		return;
	lv_group_remove_all_objs(ebook_group);
	lv_group_del(ebook_group);
	ebook_group = NULL;
	if(bar_timer)
	{
		lv_timer_pause(bar_timer);
		lv_timer_del(bar_timer);
		bar_timer = NULL;
	}
	if(updata_page_timer)
	{
		lv_timer_pause(updata_page_timer);
		lv_timer_del(updata_page_timer);
		updata_page_timer = NULL;
	}
	playlist_deinit();
	if(backstage_media_handle_get()){
		backstage_player_task_stop(0,NULL);
	}
	win_ebook_close();
	lv_obj_remove_event_cb(ui_ebook_label,ebook_keyinput_event_cb);
	lv_obj_clean(ui_ebook_txt);//del all child obj 
}

static void *read_ebook_thread_task(void *arg)
{
	char read_line[MAX_FILE_NAME] = {0};
	ebook_page_info_t *ebook_page_info = NULL;
	UINT32 read_tmp=0;
	UINT16 line_size = 0;
	UINT32 last_save_byte_size=0;
	save_node_t save_node;
	UINT32 read_size_tmp=0;
	UINT32 last_prev_offset_size = 0;
	UINT32 last_next_offset_size = 0;
	UINT8 read_direct=1;//0:prev 1:down
	UINT32 offset_block=0;
	int i;
	UINT32 prev_page_tmp=0;
	line_data_t ebook_line_info_tmp[txt_data.ebook_page_line];
	memset(ebook_line_info_tmp,0,sizeof(line_data_t)*txt_data.ebook_page_line);
	memset(&txt_data_tmp,0,sizeof(txt_ebook_data));
	memset(read_line,0,MAX_FILE_NAME);
	strcpy(read_line,arg);
	pthread_read_ebook_state = 0;
	if(txt_data.cur_fp_type ==  UTF16_LE_TYPE || txt_data.cur_fp_type ==  UTF16_BE_TYPE || 
		txt_data.cur_fp_type ==  UTF16_LE_TYPE_DIS_BOM || txt_data.cur_fp_type ==  UTF16_BE_TYPE_DIS_BOM)
	{
		while((line_size = fgetws_ex(read_line,MAX_FILE_NAME,txt_data.fp_ebook)) >0)
		{
			if(pthread_read_ebook_state == -1)
				break;
			read_size_tmp += line_size;
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				for(i=0;i<line_size/txt_data.ebook_line_max_bytes;i++)
				{
					ebook_line_info_tmp[read_tmp].line_number = read_tmp;
					txt_data_tmp.lseek_num += txt_data.ebook_line_max_bytes;
					ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
					if((read_tmp % (txt_data.ebook_page_line-1) == 0 )&& read_tmp != 0)
					{
						ebook_page_info = malloc(sizeof(ebook_page_info_t));
						ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
						ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
						last_save_byte_size = ebook_page_info->e_page_seek;
						read_tmp = 0;
						ebook_page_info->e_page_num = txt_data_tmp.page_all++;
						ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
					}
					else
						read_tmp++;
				}
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					ebook_line_info_tmp[read_tmp].line_number = read_tmp;
					txt_data_tmp.lseek_num += line_size%txt_data.ebook_line_max_bytes;
					ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
					if((read_tmp) % (txt_data.ebook_page_line-1) == 0 && read_tmp != 0)
					{
						ebook_page_info = malloc(sizeof(ebook_page_info_t));
						ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
						ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
						last_save_byte_size = ebook_page_info->e_page_seek;
						read_tmp = 0;
						ebook_page_info->e_page_num = txt_data_tmp.page_all++;
						ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
					}
					else
						read_tmp++;
				}
			}
			else
			{
				ebook_line_info_tmp[read_tmp].line_number = read_tmp;
				txt_data_tmp.lseek_num += line_size;
				ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
				if((read_tmp) % (txt_data.ebook_page_line-1) == 0 && read_tmp != 0)
				{
					ebook_page_info = malloc(sizeof(ebook_page_info_t));
					ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
					ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
					last_save_byte_size = ebook_page_info->e_page_seek;
					read_tmp = 0;
					ebook_page_info->e_page_num = txt_data_tmp.page_all++;
					ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
				}
				else
					read_tmp++;
			}
		}
		line_size = strlen(read_line);
		if(read_tmp>0 && read_tmp < txt_data.ebook_page_line)
		{
			ebook_page_info = malloc(sizeof(ebook_page_info_t));
			ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
			ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
			last_save_byte_size = ebook_page_info->e_page_seek;
			read_tmp = 0;
			ebook_page_info->e_page_num = txt_data_tmp.page_all++;
			ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
		}
	}
	else
	{
		while(fgets(read_line,MAX_FILE_NAME,txt_data.fp_ebook))
		{
			if(pthread_read_ebook_state == -1)
				break;
			line_size = strlen(read_line);
			read_size_tmp += line_size;
			if(line_size>txt_data.ebook_line_max_bytes)
			{
				for(i=0;i<line_size/txt_data.ebook_line_max_bytes;i++)
				{
					ebook_line_info_tmp[read_tmp].line_number = read_tmp;
					txt_data_tmp.lseek_num += txt_data.ebook_line_max_bytes;
					ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
					if((read_tmp % (txt_data.ebook_page_line-1) == 0 )&& read_tmp != 0)
					{
						ebook_page_info = malloc(sizeof(ebook_page_info_t));
						ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
						ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
						last_save_byte_size = ebook_page_info->e_page_seek;
						read_tmp = 0;
						ebook_page_info->e_page_num = txt_data_tmp.page_all++;
						ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
					}
					else
						read_tmp++;
				}
				if(line_size%txt_data.ebook_line_max_bytes)
				{
					ebook_line_info_tmp[read_tmp].line_number = read_tmp;
					txt_data_tmp.lseek_num += line_size%txt_data.ebook_line_max_bytes;
					ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
					if((read_tmp) % (txt_data.ebook_page_line-1) == 0 && read_tmp != 0)
					{
						ebook_page_info = malloc(sizeof(ebook_page_info_t));
						ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
						ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
						last_save_byte_size = ebook_page_info->e_page_seek;
						read_tmp = 0;
						ebook_page_info->e_page_num = txt_data_tmp.page_all++;
						ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
					}
					else
						read_tmp++;
				}
			}
			else
			{
				ebook_line_info_tmp[read_tmp].line_number = read_tmp;
				txt_data_tmp.lseek_num += line_size;
				ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
				if((read_tmp) % (txt_data.ebook_page_line-1) == 0 && read_tmp != 0)
				{
					ebook_page_info = malloc(sizeof(ebook_page_info_t));
					ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
					ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
					last_save_byte_size = ebook_page_info->e_page_seek;
					read_tmp = 0;	
					ebook_page_info->e_page_num = txt_data_tmp.page_all++;
					ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
				}
				else
					read_tmp++;
			}
		}
		line_size = strlen(read_line);
		if(read_tmp>0 && read_tmp < txt_data.ebook_page_line)
		{
			ebook_page_info = malloc(sizeof(ebook_page_info_t));
			ebook_page_info->e_page_byte = txt_data_tmp.lseek_num-last_save_byte_size;
			ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
			last_save_byte_size = ebook_page_info->e_page_seek;
			read_tmp = 0;
			ebook_page_info->e_page_num = txt_data_tmp.page_all++;
			ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
		}
	}
	if(pthread_read_ebook_state == -1)
		return NULL;
	pthread_read_ebook_state = 1;
	if(txt_data.read_offset > ((DISPLAY_BUFF_SIZE/2) *MAX_FILE_NAME))
		txt_data.cur_page = txt_data.cur_page+(txt_data.last_save_page-txt_data.save_line_num/txt_data.ebook_page_line);
	else
		txt_data.cur_page = txt_data.cur_page;
	txt_data.page_all = txt_data_tmp.page_all;
	// ebook_show_page_info(0);
	// do not show page info ,let it show in ebook_update_timer
	ebook_list = glist_nth(ebook_list,txt_data.cur_page-1);
	txt_data_tmp.page_all -= 1;
	if(read_buff_tmp)
	{
		free(read_buff_tmp);
		read_buff_tmp = NULL;
	}
}
int ebook_read_file_pthread_stop(void)
{
	if (read_ebook_thread_id)
	{
		pthread_read_ebook_state = -1;
		pthread_join(read_ebook_thread_id, NULL);
	}
	read_ebook_thread_id = 0;
	pthread_read_ebook_state = -1;

	return 0;
}

int ebook_read_file_task(char *name)
{
	pthread_attr_t attr;
	ebook_page_info_t *ebook_page_info = NULL;
	int ret = -1;
	if (pthread_read_ebook_state == 0){
		printf("%s(), line: %d. ebook_read_file_task restart!\n", __func__, __LINE__);
		return API_SUCCESS;
	}
	//create the message task
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x5000);
	//pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
	
	if(pthread_create(&read_ebook_thread_id, &attr, read_ebook_thread_task, (void*)name)) {
		pthread_attr_destroy(&attr);
		return API_FAILURE;
	}
	pthread_attr_destroy(&attr);

	updata_page_timer=lv_timer_create(ebook_updata_page_timer_cb, 1000, NULL);
	
	return ret;
}

