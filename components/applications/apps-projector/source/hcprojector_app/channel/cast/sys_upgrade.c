/*
sys_upgrade.c
 */


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hcfota.h>
#include "com_api.h"
#include "factory_setting.h"

#define USB_UPG_FILE_WILDCARD	"HCFOTA_"//"hc_upgrade"
#define MAX_UPG_LENGTH      16*1024*1024
#define READ_LENGTH      200*1024


static int mtd_upgrade_callback(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
    control_msg_t msg = {0};

    if (HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS == event){
        msg.msg_type = MSG_TYPE_UPG_BURN_PROGRESS;
        msg.msg_code = (uint32_t)param;
        api_control_send_msg(&msg);
        printf("burning %d ...\n", (int)param);

    }else if (HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS == event){
        printf("download %d ...\n", (int)param);
    }
    return 0;
}

static int sys_upg_fail_proc(int fail_ret)
{
    control_msg_t msg = {0};

    if (!fail_ret)
        return API_SUCCESS;

    msg.msg_type = MSG_TYPE_UPG_STATUS;

    switch (fail_ret)
    {
    // case HCFOTA_ERR_PRODUCT_ID:
    //     msg.msg_code = UPG_STATUS_PRODUCT_ID_MISMATCH;
    //     break;
    case HCFOTA_ERR_VERSION:
        msg.msg_code = UPG_STATUS_VERSION_IS_OLD;
        break;
    case HCFOTA_ERR_HEADER_CRC:
        msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
        break;
    case HCFOTA_ERR_PAYLOAD_CRC:
        msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
        break;
    case HCFOTA_ERR_DECOMPRESSS:
        msg.msg_code = UPG_STATUS_FILE_UNZIP_ERROR;
        break;
    case HCFOTA_ERR_UPGRADE:
        msg.msg_code = UPG_STATUS_BURN_FAIL;
        break;
    // case HCFOTA_ERR_DOWNLOAD:
    //     break;
    // case HCFOTA_ERR_LOADFOTA:
    //     break;
    default:
        break;
    }

    printf("%s(), line: %d. ugrade fail:%d\n", __func__, __LINE__, fail_ret);
    if (msg.msg_code){
        api_control_send_msg(&msg);

        if (HCFOTA_ERR_UPGRADE == fail_ret){
            api_sleep_ms(3000);        
            api_system_reboot();
        }
    }

    return API_SUCCESS;
}

int sys_upg_flash_burn(char *buff, uint32_t length)
{
    int rc;
    control_msg_t msg = {0};

    set_remote_control_disable(true);
    msg.msg_type = MSG_TYPE_UPG_STATUS;
    msg.msg_code = UPG_STATUS_BURN_START;
    api_control_send_msg(&msg);
    api_sleep_ms(200);

  #ifdef __linux__
    api_hw_watchdog_mmap_open();
  #endif

    api_upgrade_buffer_sync();
#ifdef APPMANAGER_SUPPORT //only support in linux
    (void)buff;
    (void)length;
    rc = hcfota_url_checkonly(UPGRADE_APP_TEMP_OTA_BIN, 0);
    if (0 == rc)
    {
        rc = api_app_command_upgrade(UPGRADE_APP_TEMP_OTA_BIN);
        if (0 == rc){
            //send upgrade command OK, entering upgrade app.
            while(1)
                sleep(1);
        }
    }
#else
    rc = hcfota_memory_checkonly(buff, length, 0);
    if (0 == rc)
    {
        api_enter_flash_rw();
        rc = hcfota_memory(buff, length, mtd_upgrade_callback, 0);
        api_leave_flash_rw();
    }
#endif

    msg.msg_type = MSG_TYPE_UPG_STATUS;
    if (0 == rc){
        printf("%s(), line: %d. ugrade OK, reboot now ......\n", __func__, __LINE__);
        msg.msg_code = UPG_STATUS_BURN_OK;
        api_control_send_msg(&msg);
        api_sleep_ms(3000);        
        api_system_reboot();
    }else{
        sys_upg_fail_proc(rc);
    #ifdef __linux__
        api_hw_watchdog_mmap_close();
    #endif
        set_remote_control_disable(false);
    }

    return rc;
}


