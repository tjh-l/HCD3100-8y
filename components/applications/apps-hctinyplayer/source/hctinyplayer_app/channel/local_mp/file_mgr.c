#include <string.h>
#include <linux/ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <glist.h>
#include "com_api.h"
#include "file_mgr.h"

static file_type_t file_mgr_filter_file(media_type_t media_type, char *file_name);

#define EXT_NAME_LEN    12

typedef struct
{
    char (*ext_name)[EXT_NAME_LEN];
    int ext_count;
    file_type_t file_type;
} file_ext_t;

static char m_movie_support_ext[][EXT_NAME_LEN] =
{
    // "mov",
    // "mp4",
    // "3gp",
    // "3g2",
    // "dmskm",
    // "skm",
    // "k3g",
    // "qt",
    // "mj2",
    // "psp",
    // "m4b",
    // "ism",
    // "ismv",
    // "f4v",
    // "iso",
    // "m4b",
    // "asf",
    // "wmv",
    // "avi",
    // "avs",
    // "amv",
    // "duk",
    "flv",
    // "kux",
    // "mkv",
    // "mk3d",
    // "mks",
    // "webm",
    // "mpeg",
    // "mpg",
    // "dat",
    // "vob",
    // "dvd",
    // "dv",
    // "m1v",
    // "pss",
    // "pmf",
    // "ts",
    // "m2t",
    // "m2ts",
    // "mts",
    // "rm",
    // "rmvb",
    // "ram",
    // "ogm",
    // "swf",
    // "dv",
    // "nuv",
    // "gxf",
    // "mxf",
    // "mpc",
    // "h264",
    // "vc1",
    // "m4v",
    // "pva",
    // "tp",
    // "trp",
    // "evo",
    // "asx",
};

static char m_music_support_ext[][EXT_NAME_LEN] =
{
    "dsf",
    "m4a",
    "isma",
    "wma",
    "fla",
    "mka",
    "aob",
    "ra",
    "ogg",
    "spx",
    "mp3",
    "aa",
    "act",
    "aif",
    "aiff",
    "wav",
    "ape",
    "aac",
    "adts",
    "flac",
    "ac3",
    "eac3",
    "amr",
    "au",
    "caf",
    "gsm",
    "htk",
    "ircam",
    "mlp",
    "mtv",
    "nist",
    "pvf",
    "paf",
    "rso",
    "voc",
    "fap",
    "qcp",
    "tak",
    "oma",
    "w64",
    "wv",
    "tta",
    "mp2",
    "mid",
};

static char m_photo_support_ext[][EXT_NAME_LEN] =
{
    "jpg",
    "jpe",
    "jpeg",
    "bmp",
    "gif",
    "png",
    "tga",
    "ico",
    "webp",
    "tiff",
    "tif",
    "targa",
};

static char m_txt_support_ext[][EXT_NAME_LEN] =
{
    "txt",
    "lrc",
};


#define MEDIA_FILE_DEF(x,y) \
{   \
    .ext_name = x, \
    .ext_count = HC_ARRAY_SIZE(x), \
    .file_type = y, \
}

static file_ext_t m_meida_ext[] =
{
    [MEDIA_TYPE_VIDEO] = MEDIA_FILE_DEF(m_movie_support_ext, FILE_VIDEO),
    [MEDIA_TYPE_MUSIC] = MEDIA_FILE_DEF(m_music_support_ext, FILE_MUSIC),
    [MEDIA_TYPE_PHOTO] = MEDIA_FILE_DEF(m_photo_support_ext, FILE_IMAGE),
    [MEDIA_TYPE_TXT] = MEDIA_FILE_DEF(m_txt_support_ext, FILE_TXT),
};


// compare_file_list_items
/////////////////////////////////////////////////////////////////////////////
/*
 *  Sort by Name.
 *   a < b  return <0;
 *   a==b  return 0;
 *   a > b  return > 0
 */
static int fs_compare_func(void *a, void *b, void *user_data)
{
    file_node_t *pnode = NULL;
    char *pnm1, *pnm2;
    uint8_t c1, c2;
    pnode = (file_node_t *)a;
    pnm1 = pnode->name;
    pnode = (file_node_t *)b;
    pnm2 = pnode->name;
    do
    {
        c1 = tolower(*pnm1++);
        c2 = tolower(*pnm2++);
    }
    while (c1 && c1 == c2);
    return (int)(c1 - c2);
}

//only free data,have to set data NULL
static void file_mgr_free_data(void *data, void *user_data)
{
    (void)user_data;
    if (data)
        free(data);
}

static int strcmp_c(const char *s1, const char *lower_s2)
{
    int i;
    char c1, c2;

    for (i = 0, c1 = *s1, c2 = *lower_s2; (c1 != '\0') && (c2 != '\0'); i++)
    {
        c1 = *(s1 + i);
        c2 = *(lower_s2 + i);

        if ((c1 >= 'A') && (c1 <= 'Z')) c1 += 'a' - 'A';
        //To fast compare, c2 is always lower case.
        //if ((c2 >= 'A') && (c2 <='Z')) c2 += 'a' - 'A';

        if (c1 != c2) break;
    }

    return (int)c1 - (int)c2;
}


static file_type_t get_support_media(media_type_t media_type, char *ext)
{
    int i;
    int count = 0;
    char (*support_ext)[EXT_NAME_LEN];

    count = m_meida_ext[media_type].ext_count;
    support_ext = m_meida_ext[media_type].ext_name;
    for (i = 0; i < count; i ++)
    {
        if (!strcmp_c(ext, support_ext[i]))
            return m_meida_ext[media_type].file_type;
    }
    return FILE_OTHER;
}

static file_type_t file_mgr_filter_file(media_type_t media_type, char *file_name)
{
    int i;
    int j;
    char *ext_name = NULL;
    int ret_code;

    if ( media_type != MEDIA_TYPE_VIDEO &&
         media_type != MEDIA_TYPE_MUSIC &&
         media_type != MEDIA_TYPE_PHOTO &&
         media_type != MEDIA_TYPE_TXT
       )
        return FILE_OTHER;

    for (i = 0, j = -1; file_name[i] != '\0'; i++)
    {
        if (file_name[i] == '.') j = i;
    }

    if (j == -1)
    {
        return FILE_OTHER;
    }
    else
    {
        j++;
        ext_name = file_name + j;
    }

    if (get_support_media(MEDIA_TYPE_VIDEO, ext_name) != FILE_OTHER)
        return FILE_VIDEO;
    // if (get_support_media(MEDIA_TYPE_MUSIC, ext_name) != FILE_OTHER)
    //     return FILE_MUSIC;
    // if (get_support_media(MEDIA_TYPE_PHOTO, ext_name) != FILE_OTHER)
    //     return FILE_IMAGE;

    return FILE_OTHER;
}

void file_mgr_get_fullname(char *fullname, char *path, char *name)
{
    strcpy(fullname, path);
    strcat(fullname, "/");
    strcat(fullname, name);
}

file_node_t *file_mgr_get_file_node(file_list_t *file_list, int index)
{
    glist *plist = NULL;
    plist = (glist *)file_list->list;
    file_node_t *file_node = NULL;

    if (index > (file_list->dir_count + file_list->file_count))
        return (void *)API_FAILURE;

    file_node = glist_nth_data(plist, index);
    if (NULL != file_node)
    {
        return file_node;
    }
    else
    {
        printf("can not find the file node via the index:%d\n", index);
        return NULL;
    }
}

static glist *subtitle_glist = NULL;
/**
 * @description: add a subtitile file filter ,support file include .ass .ssa .srt .txt .sub .smi
 * @param {char*} file_name
 * @return {*} true is subtitle file ,false is not
 * @author: Yanisin
 */
static bool file_mgr_subtitle_filter(char *file_name)
{
    int i;
    int j;
    char *ext_name = NULL;

    for (i = 0, j = -1; file_name[i] != '\0'; i++)
    {
        if (file_name[i] == '.') j = i;
    }

    if (j == -1)
    {
        return false;
    }
    else
    {
        j++;
        ext_name = file_name + j;
    }
    if ( strcasestr(ext_name, "ass") ||
         strcasestr(ext_name, "ssa") ||
         strcasestr(ext_name, "idx") ||
         strcasestr(ext_name, "sub") ||
         strcasestr(ext_name, "smi") ||
         strcasestr(ext_name, "sami") ||
         strcasestr(ext_name, "txt") ||
         strcasestr(ext_name, "mpl2") ||
         strcasestr(ext_name, "srt") ||
         strcasestr(ext_name, "vtt") ||
         strcasestr(ext_name, "lrc") ||
         strcasestr(ext_name, "mpl")
       )
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool file_mgr_subtitile_list_create(file_list_t *src_list, char *name)
{
    if (src_list->media_type == MEDIA_TYPE_VIDEO || src_list->media_type == MEDIA_TYPE_MUSIC)
    {
        if (file_mgr_subtitle_filter(name))
        {
            char *subtitle_file = strdup(name);
            subtitle_glist = glist_append(subtitle_glist, subtitle_file);
            return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

#define LimitFile_Num 1000
int file_mgr_create_list(file_list_t *file_list, char *path)
{
    DIR *dirp;
    struct dirent *entry;
    file_node_t *file_node = NULL;
    file_type_t file_type;
    ASSERT_API(file_list);
    file_list_t file_list_dir;
    int len;
    if ((dirp = opendir(path)) == NULL)
    {
        control_msg_t ctl_msg = {0};
        partition_info_t *p_info = mmp_get_partition_info();
        char *devname = p_info->used_dev;
        ctl_msg.msg_type = MSG_TYPE_USB_UNMOUNT;
        ctl_msg.msg_code = (uint32_t)devname;
        api_control_send_msg(&ctl_msg);
        printf(">>!%s ,%d\n", __func__, __LINE__);
        return API_FAILURE;
    }

    //step 1: free current file list's node
    if (file_list->list)
    {
        glist_free_full(file_list->list, file_mgr_free_data);
        file_list->dir_count = 0;
        file_list->file_count = 0;
        file_list->list = NULL;
    }
    strcpy(file_list->dir_path, path);
    strcpy(file_list_dir.dir_path, path);
    file_list_dir.media_type = FILE_DIR;//dir
    file_list_dir.file_count = 0;
    file_list_dir.dir_count = 0;
    file_list_dir.list = NULL;
    if (subtitle_glist)
    {
        glist_free_full(subtitle_glist, file_mgr_free_data);
        subtitle_glist = NULL;
    }

    //step 2: create the new file list via scan the dir
    while (1)
    {
        entry = readdir(dirp);
        if (!entry)
            break;

        if (!strcmp(entry->d_name, "."))
        {
            //skip the upper dir
            continue;
        }

        if (!strcmp(entry->d_name, ".."))
        {
            //skip the upper dir
            continue;
        }

        len = strlen(entry->d_name);
        if (entry->d_type == 4)
        {
            //dir
            file_node = (file_node_t *)malloc(sizeof(file_node_t) + len + 1);
            memset(file_node, 0, sizeof(file_node_t));
            file_node->type = FILE_DIR;
            strcpy(file_node->name, entry->d_name);
            file_list->dir_count ++;
            file_list_dir.dir_count ++;
            file_list_dir.list = glist_prepend(file_list_dir.list, (void *)file_node);
        }
        else
        {
            file_type = file_mgr_filter_file(file_list->media_type, entry->d_name);
            if (FILE_OTHER == file_type)
            {
                if (file_mgr_subtitile_list_create(file_list, entry->d_name) == true)
                {
                    printf(">>! %s, add in subtutle list\n", entry->d_name);
                }
                continue;
            }

            file_node = (file_node_t *)malloc(sizeof(file_node_t) + len + 1);
            memset(file_node, 0, sizeof(file_node_t));
            file_node->type = file_type;
            file_list->file_count ++;
            strcpy(file_node->name, entry->d_name);
            file_list->list = glist_prepend(file_list->list, (void *)file_node);
        }

        //add 1000 files to limit
        if (file_list->dir_count + file_list->file_count == LimitFile_Num)
            break;
    }

    //step 3: post process the list, for example, sort the file list. to be done
    file_list->list = glist_sort(file_list->list, fs_compare_func, NULL); // sort by Name
    file_list_dir.list = glist_sort(file_list_dir.list, fs_compare_func, NULL);
    for (int i = file_list_dir.dir_count - 1; i >= 0; i--)
    {
        file_node = glist_nth_data(file_list_dir.list, i);
        file_list->list = glist_prepend(file_list->list, (void *)file_node);
    }
    file_node = (file_node_t *)malloc(sizeof(file_node_t) + 3 + 1);
    memset(file_node, 0, sizeof(file_node_t));
    file_node->type = FILE_DIR;
    file_list->dir_count ++;
    strcpy(file_node->name, "..");
    // insert ".." as list head after sorted.
    file_list->list = glist_prepend(file_list->list, (void *)file_node);

    if (dirp != NULL)
    {
        closedir(dirp);
        dirp = NULL;
    }

    printf("current dir: %s, file: %d, dir: %d\n", path, file_list->file_count, file_list->dir_count);
    return API_SUCCESS;
}

/*free mem list */
int file_mgr_free_list(file_list_t *file_list)
{
    if (file_list == NULL)
        return 0;
    if (file_list->list)
    {
        glist_free_full(file_list->list, file_mgr_free_data);
        file_list->dir_count = 0;
        file_list->file_count = 0;
        file_list->list = NULL;
        memset(file_list, 0, sizeof(file_list_t));
        printf("free media_list struct  memory\n");
    }
    return 0;
}
