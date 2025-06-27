/*
data_mgr.c
used for save the config data, and read config data from nonvolatile memory
(nor/nand flash)
 */
#include <sys/types.h>//define pid_t ande size_t
#include <sys/stat.h>
#include <hcuapi/dis.h>
#include <hcuapi/persistentmem.h>
#include <sys/ioctl.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include "com_api.h"
#include "data_mgr.h"
#include "tv_sys.h"

static app_data_t m_app_data;
static app_data_t m_app_data_bak;

static sys_data_t m_sys_data;
static sys_data_t m_sys_data_bak;

#define SYS_DATA_DEV    "/dev/persistentmem"

//check the system data if valid
static bool data_is_valid(uint8_t *buff, uint32_t length)
{
#if 0    
    int i;
    uint8_t *p = buff;
    printf("sys data length: %d\n", length);
    for (i = 0; i < length; i ++){
        printf("0x%.2x\n", *p++);
    }
#endif
    uint32_t read_crc32 = 0;
    uint32_t calc_crc32 = 0;

    memcpy((void*)&read_crc32, (void*)buff, CRC_LENGTH);
    //printf("read_crc32: 0x%x\n", read_crc32);

    calc_crc32 = api_crc32(0xFFFFFFFF, buff+CRC_LENGTH, length-CRC_LENGTH);
    //printf("calc_crc32: 0x%x\n", calc_crc32);
    if (read_crc32 == calc_crc32)
        return true;
    else
        return false;

}

static void data_mgr_default_init(void)
{
    printf("%s(), line:%d. reset data!\n", __FUNCTION__, __LINE__);

    m_app_data.resolution = APP_TV_SYS_AUTO;//APP_TV_SYS_1080P;
    m_app_data.mirror_rotation = 0;//default disable.
    m_app_data.mirror_vscreen_auto_rotation = 1;//default enable.

    m_sys_data.tvtype = TV_LINE_1080_60;
    m_sys_data.volume = 60;
    m_sys_data.flip_mode = FLIP_MODE_REAR;

    m_app_data.um_full_screen = 0;

    memset(m_app_data.ium_pair_data, 0, sizeof(m_app_data.ium_pair_data));
    memset(m_app_data.ium_uuid, 0, sizeof(m_app_data.ium_uuid));
    m_app_data.ium_pdata_len = 0;
}

static int data_mgr_reset(void)
{
    memset(&m_app_data, 0, sizeof(app_data_t));
    memset(&m_sys_data, 0, sizeof(sys_data_t));

    m_sys_data.firmware_version = 2008080808;
    strcpy(m_sys_data.product_id, "HC_DEMO_BOARD");

    data_mgr_default_init();
    return API_SUCCESS;
}

int data_mgr_load(void)
{
    int fd_sys = -1;
    app_data_t *p_app_data_tmp = NULL;
    sys_data_t *p_sys_data_tmp = NULL;
    int ret = 0;
    int sys_re_save = 0;

    struct persistentmem_node_create new_node;
    struct persistentmem_node node;

    data_mgr_reset();

    fd_sys = open(SYS_DATA_DEV, O_SYNC | O_RDWR);
    if (fd_sys < 0)
    {
        printf("Open %s fail!\n", SYS_DATA_DEV);
        return API_FAILURE;
    }

    p_app_data_tmp = (app_data_t*)malloc(sizeof(app_data_t));
    p_sys_data_tmp = (sys_data_t*)malloc(sizeof(sys_data_t));
    memset(p_app_data_tmp, 0, sizeof(app_data_t));
    memset(p_sys_data_tmp, 0, sizeof(sys_data_t));


#if 0
    //Read the persistentmem first time, for it has no valid data, it should be fail.
    int length;
    length = read(fd_sys, (void*)(p_app_data_tmp), sizeof(app_data_t));
    if (length != sizeof(app_data_t)){
        printf("read error!\n");
    }

    //if the sys data is valid, use the sys data from flash
    if (data_is_valid(p_app_data_tmp, sizeof(app_data_t))){
        memcpy(&m_app_data, p_app_data_tmp, sizeof(app_data_t));
        printf("%s(), line:%d. system data is OK!\n", __FUNCTION__, __LINE__);
    }
    else{
        printf("%s(), line:%d. system data is invalid!\n", __FUNCTION__, __LINE__);
    }
    memcpy(&m_app_data_bak, &m_app_data, sizeof(app_data_t));
#endif

    //load sys data
    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(sys_data_t);
    node.buf = p_sys_data_tmp;

    if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
        new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        new_node.size = sizeof(sys_data_t);
        printf("%s(), line:%d. Get sys data Node failed, Create new sys node!\n", __func__, __LINE__);                    
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
            printf("Create sys data Node failed\n");
            ret = API_FAILURE;
            goto load_exit;
        }

        //save system data to flash.
        node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        node.offset = 0;
        node.size = sizeof(sys_data_t);
        node.buf = &m_sys_data;
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
            printf("%s(), line:%d. put sys data failed\n", __func__, __LINE__);            
            ret = API_FAILURE;
            goto load_exit;
        }
    }else{
        memcpy(&m_sys_data, p_sys_data_tmp, sizeof(sys_data_t));  
        printf("%s(), line:%d. Get sys data Node OK, volume:%d!\n", __func__, __LINE__, m_sys_data.volume);                

        if (0 == m_sys_data.firmware_version){
            int count = 0;
            while(1){
                if (count ++ < 100){
                    printf("\n========= %s(), line:%d. fw ver can not be 0, ERROR!!!!!!=============\n", 
                        __func__, __LINE__);
                }
                api_sleep_ms(500);
            }
        }

        if ((int8_t)(m_sys_data.volume) <= 0 || (int8_t)(m_sys_data.volume) > 100){
            m_sys_data.volume = 60;
            sys_re_save = 1;
        }
        if ((int)(m_sys_data.tvtype) <= 0){
            m_sys_data.tvtype = TV_LINE_1080_60;
            sys_re_save = 1;
        }
        if (sys_re_save){
            printf("%s(), line:%d. set sys data!\n", __FUNCTION__, __LINE__);
            node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
            node.offset = 0;
            node.size = sizeof(sys_data_t);
            node.buf = &m_sys_data;
            if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
                printf("%s(), line:%d. put sys data failed\n", __func__, __LINE__);            
                ret = API_FAILURE;
                goto load_exit;
            }
        }

    }
    memcpy(&m_sys_data_bak, &m_sys_data, sizeof(sys_data_t));

    //load app data
    node.id = PERSISTENTMEM_NODE_ID_CASTAPP;
    node.offset = 0;
    node.size = sizeof(app_data_t);
    node.buf = p_app_data_tmp;
    if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
        printf("%s(), line:%d. Get app data Node failed, Create app new node!\n", __func__, __LINE__);        
        new_node.id = PERSISTENTMEM_NODE_ID_CASTAPP;
        new_node.size = sizeof(app_data_t);
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
            printf("Create app data Node failed\n");
            ret = API_FAILURE;
            goto load_exit;
        }

        //save app data to flash.
        m_app_data.data_crc32 = 0x12345678;
        node.id = PERSISTENTMEM_NODE_ID_CASTAPP;
        node.offset = 0;
        node.size = sizeof(app_data_t);
        node.buf = &m_app_data;
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
            ret = API_FAILURE;
            goto load_exit;
        }
    }else{
        printf("%s(), line:%d. Get app data Node OK!\n", __func__, __LINE__);                
        memcpy(&m_app_data, p_app_data_tmp, sizeof(app_data_t));       
    }
    memcpy(&m_app_data_bak, &m_app_data, sizeof(app_data_t));


load_exit:
    if (p_sys_data_tmp)
        free((void*)p_sys_data_tmp);

    if (p_app_data_tmp)
        free((void*)p_app_data_tmp);

    if (fd_sys >= 0)
        close(fd_sys);


    return ret;
}

app_data_t *data_mgr_app_get(void)
{
    return &m_app_data;
}

sys_data_t *data_mgr_sys_get(void)
{
    return &m_sys_data;
}

int data_mgr_save(void)
{
    int fd_sys = -1;
    int app_data_save = 1;
    int sys_data_save = 1;
    struct persistentmem_node node;
    int ret = API_SUCCESS;

    if (0 == memcmp(&m_app_data_bak, &m_app_data, sizeof(app_data_t)))
        app_data_save = 0;

    if (0 == memcmp(&m_sys_data_bak, &m_sys_data, sizeof(sys_data_t)))
        sys_data_save = 0;


    if (0 == m_sys_data.firmware_version){
        printf("\n\n========= %s(), line:%d. fw ver can not be 0, ERROR!!!!!!=============\n\n", 
            __func__, __LINE__);
    }

    if (!app_data_save && !sys_data_save)
        return API_SUCCESS;

    fd_sys = open(SYS_DATA_DEV, O_SYNC | O_RDWR);
    if (fd_sys < 0){
        printf("Open %s fail!\n", SYS_DATA_DEV);
        return API_FAILURE;
    }

#if 0
//not save all system data.
    if (sys_data_save){
        //save sys data to flash.
        node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        node.offset = 0;
        node.size = sizeof(sys_data_t);
        node.buf = &m_sys_data;
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
            printf("%s(), line:%d. put sys data failed\n", __func__, __LINE__);
            ret = API_FAILURE;
            goto save_exit;
        }
    }
    memcpy(&m_sys_data_bak, &m_sys_data, sizeof(sys_data_t));
#endif    

    if (app_data_save){
        //save app data to flash.
        node.id = PERSISTENTMEM_NODE_ID_CASTAPP;
        node.offset = 0;
        node.size = sizeof(app_data_t);
        node.buf = &m_app_data;
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
            printf("%s(), line:%d. put app data failed\n", __func__, __LINE__);
            ret = API_FAILURE;
            goto save_exit;
        }
    }
    memcpy(&m_app_data_bak, &m_app_data, sizeof(app_data_t));


#if 0
    //calc the data crc
    uint8_t *p;
    uint32_t calc_crc32 = 0;;
    int length;
    p = (uint8_t*)(&m_app_data) + CRC_LENGTH;
    calc_crc32 = api_crc32(0xFFFFFFFF, p, sizeof(app_data_t)-CRC_LENGTH);
    memcpy((void*)(&m_app_data.data_crc32), (void*)(&calc_crc32), CRC_LENGTH);
    // printf("%s(), line:%d. calc_crc32=0x%x\n", __FUNCTION__,__LINE__,calc_crc32);

    length = write(fd_sys, (void*)(&m_app_data), sizeof(app_data_t));
    if (length != sizeof(app_data_t)){
        printf("write error!\n");
        return API_FAILURE;
    }
#endif


save_exit:

    if (fd_sys >= 0)
        close(fd_sys);
    
    return ret;
}

static int data_mgr_item_save(uint16_t offset, uint16_t size, int node_id)
{
    uint8_t *save_item = NULL;
    uint8_t *bak_item = NULL;

    if (PERSISTENTMEM_NODE_ID_SYSDATA == node_id){
        save_item = (uint8_t*)(&m_sys_data) + offset;
        bak_item = (uint8_t*)(&m_sys_data_bak) + offset;
    }else if (PERSISTENTMEM_NODE_ID_CASTAPP == node_id){
        save_item = (uint8_t*)(&m_app_data) + offset;
        bak_item = (uint8_t*)(&m_app_data_bak) + offset;
    }else{
        return API_FAILURE;
    }

    if (0 == memcmp(save_item, bak_item, size)){
        return API_SUCCESS;
    }

    if (0 == m_sys_data.firmware_version){
        printf("\n\n========= %s(), line:%d. fw ver can not be 0, offset:%d. ERROR!!!!!!=============\n\n", 
            __func__, __LINE__, offset);
    }

    int fd_sys = -1;
    struct persistentmem_node node;

    fd_sys = open(SYS_DATA_DEV, O_SYNC | O_RDWR);
    if (fd_sys < 0){
        printf("%s(), line:%d. Open %s fail!\n", __func__, __LINE__, SYS_DATA_DEV);
        return API_FAILURE;
    }

    memset(&node, 0, sizeof(struct persistentmem_node));
    node.id = node_id;
    node.offset = offset;
    node.size = size;
    node.buf = save_item;
    if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
        printf("%s(), line:%d. put app data failed\n", __func__, __LINE__);
        close(fd_sys);
        return API_FAILURE;
    }
    close(fd_sys);

    memcpy(bak_item, save_item, size);

    return API_SUCCESS;
}

app_tv_sys_t data_mgr_app_tv_sys_get(void)
{
    return m_app_data.resolution;
}

int data_mgr_de_tv_sys_get(void)
{
    return m_sys_data.tvtype;
}

int data_mgr_flip_mode_get(void)
{
    return m_sys_data.flip_mode;
}

void data_mgr_app_tv_sys_set(app_tv_sys_t app_tv_sys)
{
    m_app_data.resolution = app_tv_sys;
    data_mgr_item_save(offsetof(app_data_t, resolution), 
        sizeof(m_app_data.resolution), PERSISTENTMEM_NODE_ID_CASTAPP);
}

void data_mgr_de_tv_sys_set(int tv_type)
{
    m_sys_data.tvtype = tv_type;
    data_mgr_item_save(offsetof(sys_data_t, tvtype), 
        sizeof(m_sys_data.tvtype), PERSISTENTMEM_NODE_ID_SYSDATA);
}

void data_mgr_factory_reset(void)
{
    int fd_sys = -1;
    int node_id = PERSISTENTMEM_NODE_ID_CASTAPP;

    printf("%s(), line:%d\n", __func__, __LINE__);

    //step 1: reset some config for system data node.
    m_sys_data.tvtype = TV_LINE_1080_60;
    m_sys_data.volume = 60;
    data_mgr_item_save(offsetof(sys_data_t, tvtype), 
        sizeof(m_sys_data.tvtype), PERSISTENTMEM_NODE_ID_SYSDATA);
    data_mgr_item_save(offsetof(sys_data_t, volume), 
        sizeof(m_sys_data.volume), PERSISTENTMEM_NODE_ID_SYSDATA);

    //step 2: delete application node.
    fd_sys = open(SYS_DATA_DEV, O_SYNC | O_RDWR);
    if (fd_sys < 0){
        printf("%s(), line:%d. Open %s fail!\n", __func__, __LINE__, SYS_DATA_DEV);
        return;
    }

    if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_DELETE, node_id) < 0) {
        printf("%s(), line:%d. delete app data failed\n", __func__, __LINE__);
    }

    close(fd_sys);
}

enum TVTYPE data_mgr_tv_type_get(void)
{
    return m_sys_data.tvtype;
}

uint8_t data_mgr_volume_get(void)
{
    return m_sys_data.volume;
}

void data_mgr_volume_set(uint8_t vol)
{
    m_sys_data.volume = vol;
    data_mgr_item_save(offsetof(sys_data_t, volume), 
        sizeof(m_sys_data.volume), PERSISTENTMEM_NODE_ID_SYSDATA);
}

void data_mgr_cast_rotation_set(int rotate)
{
    m_app_data.mirror_rotation = rotate;
    data_mgr_item_save(offsetof(app_data_t, mirror_rotation), 
        sizeof(m_app_data.mirror_rotation), PERSISTENTMEM_NODE_ID_CASTAPP);
}

void data_mgr_flip_mode_set(int flip_mode)
{
    m_sys_data.flip_mode = flip_mode;
    data_mgr_item_save(offsetof(sys_data_t, flip_mode), 
        sizeof(m_sys_data.flip_mode), PERSISTENTMEM_NODE_ID_SYSDATA);
}

int data_mgr_cast_rotation_get(void)
{
    return m_app_data.mirror_rotation;
}

int data_mgr_cast_full_screen_get(void)
{
    return m_app_data.um_full_screen;
}

void data_mgr_cast_full_screen_set(int enable)
{
    m_app_data.um_full_screen = enable;
    data_mgr_item_save(offsetof(app_data_t, um_full_screen), 
        sizeof(m_app_data.um_full_screen), PERSISTENTMEM_NODE_ID_CASTAPP);
}

void data_mgr_ium_uuid_set(char* uuid)
{
    memcpy(m_app_data.ium_uuid, uuid, sizeof(m_app_data.ium_uuid));
    data_mgr_item_save(offsetof(app_data_t, ium_uuid), 
        sizeof(m_app_data.ium_uuid), PERSISTENTMEM_NODE_ID_CASTAPP);
}

void data_mgr_ium_pair_data_set(char* ium_pair_data)
{
    memcpy(m_app_data.ium_pair_data, ium_pair_data, sizeof(m_app_data.ium_pair_data));
    data_mgr_item_save(offsetof(app_data_t, ium_pair_data), 
        sizeof(m_app_data.ium_pair_data), PERSISTENTMEM_NODE_ID_CASTAPP);
}

void data_mgr_ium_pdata_len_set(int ium_pdata_len)
{
    m_app_data.ium_pdata_len = ium_pdata_len;
    data_mgr_item_save(offsetof(app_data_t, ium_pdata_len), 
        sizeof(m_app_data.ium_pdata_len), PERSISTENTMEM_NODE_ID_CASTAPP);
}

