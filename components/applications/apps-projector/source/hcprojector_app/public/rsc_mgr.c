/*
rsc_mgr.c: use to manage the UI resource: string, font, etc
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
//#include "local_mp_ui.h"
//#include "setup.h"
#include "factory_setting.h"

#include "hcstring_id.h"
#include "../ui_rsc/string/str_english.h"
#include "../ui_rsc/string/str_chinese.h"
//#include "../ui_rsc/string/str_french.h"
#if (SUPPORT_OSD_TCHINESE==1) 
#include "../ui_rsc/string/str_Traditional_Chinese.h"
#endif 
#if (SUPPORT_OSD_FRENCH==1) 
#include "../ui_rsc/string/str_french.h"
#endif  
#if (SUPPORT_OSD_GERMAN==1) 
#include "../ui_rsc/string/str_german.h"
#endif   
#if (SUPPORT_OSD_SPANISH==1) 
#include "../ui_rsc/string/str_spanish.h"
#endif 
#if (SUPPORT_OSD_PORTUGUESE==1) 
#include "../ui_rsc/string/str_Portuguese.h"
#endif 
#if (SUPPORT_OSD_ITALIAN==1) 
#include "../ui_rsc/string/str_Italian.h"
#endif 
#if (SUPPORT_OSD_POLISH==1) 
#include "../ui_rsc/string/str_Polish.h"
#endif 
#if (SUPPORT_OSD_SWEDISH==1) 
#include "../ui_rsc/string/str_Swedish.h"
#endif 
#if (SUPPORT_OSD_FINNISH==1) 
#include "../ui_rsc/string/str_Finnish.h"
#endif 
#if (SUPPORT_OSD_GREEK==1) 
#include "../ui_rsc/string/str_Greek.h"
#endif 
#if (SUPPORT_OSD_DANISH==1) 
#include "../ui_rsc/string/str_Danish.h"
#endif
#if (SUPPORT_OSD_NORWEGIAN==1) 
    #include "../ui_rsc/string/str_norwegian.h"
#endif 
#if (SUPPORT_OSD_HUNGARY==1) 
    #include "../ui_rsc/string/str_Hungary.h"
#endif 
#if (SUPPORT_OSD_HEBREW==1) 
    #include "../ui_rsc/string/str_Hebrew.h"
#endif 
#if (SUPPORT_OSD_RUSSIAN==1)
    #include "../ui_rsc/string/str_Russian.h"
#endif 
#if (SUPPORT_OSD_VIETNAMESE==1) 
    #include "../ui_rsc/string/str_Vietnamese.h"
#endif 
#if (SUPPORT_OSD_THAI==1) 
    #include "../ui_rsc/string/str_Thai.h"
#endif
#if (SUPPORT_OSD_ARABIC==1) 
#include "../ui_rsc/string/str_Arabic.h"
#endif 
#if (SUPPORT_OSD_JAPANESE==1) 
#include "../ui_rsc/string/str_Japanese.h"
#endif 
#if (SUPPORT_OSD_KOREAN==1) 
#include "../ui_rsc/string/str_Korean.h"
#endif 
#if (SUPPORT_OSD_INDONESIAN==1) 
#include "../ui_rsc/string/str_Indonesian.h"
#endif 
#if (SUPPORT_OSD_DUTCH==1) 
#include "../ui_rsc/string/str_Netherlands.h"
#endif 
#if (SUPPORT_OSD_TURKEY==1) 
#include "../ui_rsc/string/str_Turkey.h"
#endif 

typedef struct{
    uint16_t lang_id;
    uint8_t *string_array;
}string_table_t;

string_table_t multi_string_array[] = {
    {LANGUAGE_ENGLISH, strs_array_english},
    {LANGUAGE_CHINESE, strs_array_chinese},
#if (SUPPORT_OSD_TCHINESE==1)     
    {LANGUAGE_TCHINESE, strs_array_Traditional_Chinese},
#endif      
#if (SUPPORT_OSD_FRENCH==1) 
    {LANGUAGE_FRENCH, strs_array_french},        
#endif  
#if (SUPPORT_OSD_GERMAN==1) 
    {LANGUAGE_GERMAN, strs_array_german},        
#endif  
#if (SUPPORT_OSD_SPANISH==1) 
    {LANGUAGE_SPANISH, strs_array_spanish},        
#endif      
#if (SUPPORT_OSD_PORTUGUESE==1) 
    {LANGUAGE_PORTUGUESE, strs_array_Portuguese},        
#endif
#if (SUPPORT_OSD_ITALIAN==1) 
    {LANGUAGE_ITALIAN, strs_array_Italian},
#endif 
#if (SUPPORT_OSD_RUSSIAN==1) 
    {LANGUAGE_RUSSIAN, strs_array_Russian},
#endif

#if (SUPPORT_OSD_POLISH==1) 
    {LANGUAGE_POLISH, strs_array_Polish},
#endif 
#if (SUPPORT_OSD_SWEDISH==1) 
    {LANGUAGE_SWEDISH, strs_array_Swedish},
#endif 
#if (SUPPORT_OSD_FINNISH==1) 
    {LANGUAGE_FINNISH, strs_array_Finnish},
#endif
#if (SUPPORT_OSD_GREEK==1) 
    {LANGUAGE_GREEK, strs_array_Greek},
#endif
#if (SUPPORT_OSD_DANISH==1) 
    {LANGUAGE_DANISH, strs_array_Danish},
#endif

#if (SUPPORT_OSD_NORWEGIAN==1) 
    {LANGUAGE_NORWEGIAN, strs_array_norwegian}, 
#endif 
#if (SUPPORT_OSD_HUNGARY==1) 
    {LANGUAGE_HUNGARY, strs_array_Hungary},
#endif 
#if (SUPPORT_OSD_HEBREW==1) 
    {LANGUAGE_HEBREW, strs_array_Hebrew},
#endif 
#if (SUPPORT_OSD_RUSSIAN==1)
    {LANGUAGE_RUSSIAN, strs_array_Russian},
#endif 
#if (SUPPORT_OSD_VIETNAMESE==1) 
    {LANGUAGE_VIETNAMESE, strs_array_Vietnamese},
#endif 
#if (SUPPORT_OSD_THAI==1) 
    {LANGUAGE_THAI, strs_array_Thai},
#endif


#if (SUPPORT_OSD_ARABIC==1) 
    {LANGUAGE_ARABIC, strs_array_Arabic},
#endif 
#if (SUPPORT_OSD_JAPANESE==1) 
    {LANGUAGE_JAPANESE, strs_array_Japanese},
#endif 
#if (SUPPORT_OSD_KOREAN==1) 
    {LANGUAGE_KOREAN, strs_array_Korean},
#endif 
#if (SUPPORT_OSD_INDONESIAN==1) 
    {LANGUAGE_INDONESIAN, strs_array_Indonesian},
#endif 
#if (SUPPORT_OSD_DUTCH==1) 
    {LANGUAGE_DUTCH, strs_array_Netherlands},
#endif 
#if (SUPPORT_OSD_TURKEY==1) 
    {LANGUAGE_TURKEY, strs_array_Turkey},
#endif 
#ifdef ARABIC_SUPPORT
//    {LANGUAGE_ARABIC, str_arabic_array},
#endif
};

char *api_rsc_string_get_by_langid(uint16_t lang_id, uint32_t string_id){
    uint32_t str_num = 0;
    uint8_t *str_array = NULL;
    char *str_data = NULL;
    uint32_t  str_count = 0;
    uint32_t offset = 0;
    uint32_t string_idx = string_id-1;
    int i;
    uint16_t language_id;

    int lang_count = sizeof(multi_string_array)/sizeof(multi_string_array[0]);
    for (i = 0; i<lang_count; i++)
    {
        if (lang_id == multi_string_array[i].lang_id)
        {
            str_array = multi_string_array[i].string_array;
            break;
        }
    }

    if (NULL == str_array)
        return NULL;

    language_id = (str_array[0] << 8) + str_array[1];
    str_num = (str_array[2] << 8) + str_array[3];
    if (string_idx >= str_num)
        return NULL;

	offset = (str_array[4 + 3 * string_idx] << 16) + (str_array[4 + 3 * string_idx + 1] << 8) + str_array[4 + 3 * string_idx + 2];
	str_data = (char*)&str_array[offset];

	return str_data; //utf-8, end by '\0'  
}

char *api_rsc_string_get(uint32_t string_id)
{
    uint16_t lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
//    uint16_t lang_id = LANG_CHINESE;
    return api_rsc_string_get_by_langid(lang_id, string_id);

}

