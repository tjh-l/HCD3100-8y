#include "app_config.h"
#ifdef  HDMI_RX_CEC_SUPPORT
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <pthread.h>

#include "com_api.h"
#include "app_log.h"
#include "screen.h"
#include "hdmi_rx.h"

#include <hudi/hudi_cec.h>
#include "factory_setting.h"

typedef struct
{
    int polling_stop;
    int dev_is_active; // 1:cec device as active source ; 0: not as active source
    int boardcast_en;
    hudi_handle handle;
    pthread_t pthread_id;
    hdmirx_cec_device_res_t device_res;
} hdmirx_cec_info_t;

static hdmirx_cec_info_t g_hdmirx_cec_info = {0};

static int _hdmirx_cec_report_power_stauts(hudi_cec_la_e la)
{
    hudi_cec_datapacket_t params = {0};
    return hudi_cec_send_special_command(g_hdmirx_cec_info.handle, la, HUDI_CEC_OPCODE_REPORT_POWER_STATUS, &params);
}

static int _hdmirx_cec_feature_abort(hudi_cec_la_e la)
{
    hudi_cec_datapacket_t params = {0};
    return hudi_cec_send_special_command(g_hdmirx_cec_info.handle, la, HUDI_CEC_OPCODE_FEATURE_ABORT, &params);
}

static int _hdmirx_cec_send_vendor_cmd(void *data)
{
    return hudi_cec_send_special_command(g_hdmirx_cec_info.handle, projector_get_some_sys_param(P_CEC_DEVICE_LA), HUDI_CEC_OPCODE_VENDOR_COMMAND, data);
}

int hdmirx_cec_dev_active_status_get(void)
{
    return g_hdmirx_cec_info.dev_is_active;
}

int hdmirx_cec_dev_active_status_set(int status)
{

    g_hdmirx_cec_info.dev_is_active = status;
    return 0;
}

void * hdmirx_cec_handle_get(void)
{
    return g_hdmirx_cec_info.handle;
}

void * hdmirx_cec_device_res_get(void)
{
    return &g_hdmirx_cec_info.device_res;
}

int hdmirx_cec_boardcast_en_set(int en)
{
    g_hdmirx_cec_info.boardcast_en = en;
}

int hdmirx_cec_boardcast_en_get(void)
{
    return g_hdmirx_cec_info.boardcast_en;
}

int hdmirx_cec_device_scan(hudi_handle handle, hdmirx_cec_device_res_t *res)
{
    int ret = -1;
    hudi_cec_logical_addresses_t laes = {0};

    ret = hudi_cec_scan_devices(handle ,&laes);
    if(ret < 0 || !res)
    {
        return ret ;
    }


    for (int i = 0; i < HUDI_CEC_DEVICE_BROADCAST; i++)
    {
        if(laes.addresses[i] >= 1)
        {
            res->device_la[res->count] = i;
            res->count++;
        }
    }

    return ret;
}

int hdmirx_cec_poweron_device(hudi_handle handle, hdmirx_cec_device_res_t res)
{
    int ret = -1;
    hudi_cec_power_status_e status  = HUDI_CEC_POWER_STATUS_UNKNOWN;
    for (int i = 0; i < res.count; i++)
    {
        ret = hudi_cec_get_device_power_status(handle,res.device_la[i],&status, 3000);
        if(ret < 0)
        {
            printf("%s  %d  ret : %d status:%d \n", __func__, __LINE__, ret, status);
            return ret;
        }
        if(status == HUDI_CEC_POWER_STATUS_STANDBY)
        {
            ret = hudi_cec_poweron_device(handle, res.device_la[i]);
            printf("%s  %d  addr:%d \n", __func__ , __LINE__, res.device_la[i]);
        }
    }
    
    return ret;
}

int hdmirx_cec_device_active_status_report(bool bactive)
{
    hudi_cec_datapacket_t packet = {0};

    packet.size = 2;
    if (bactive)
    {
        packet.data[0]  = 0x50;
        packet.data[1]  = 0x49;
    }
    else
    {
        packet.data[0]  = 0x50;
        packet.data[1]  = 0x4f;
    }
    printf("@@ send cmd-- gtv is %s cur channel\n", bactive ? "in" : "not in");
    return _hdmirx_cec_send_vendor_cmd(&packet);
}

int hdmirx_cec_active_device_sys_param_save(hudi_cec_logical_addresses_t *laes)
{
    int ret = -1;
    for (int la = 1; la < HUDI_CEC_DEVICE_BROADCAST; la++)
    {
        if (laes->addresses[la] > 1)
        {
            projector_set_some_sys_param(P_CEC_DEVICE_LA, la);
            projector_sys_param_save();
            ret = 0;
            break;
        }
    }
    return ret;
}

void hdmirx_cec_scan_device_save(hdmirx_cec_device_res_t *res)
{
    memcpy(&g_hdmirx_cec_info.device_res, res, sizeof(hdmirx_cec_device_res_t));
}

static void *hdmirx_cec_msg_task(void *pvParameters)
{
    hudi_cec_cmd_t cmd  = {};
    int tick_count = 0;
    int tick_timeout = 0;

    while (!g_hdmirx_cec_info.polling_stop)
    {
        if (hudi_cec_msg_receive(g_hdmirx_cec_info.handle, &cmd, true) == 0)
        {
            printf("\t[MSG in]: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", cmd.initiator, cmd.destination, cmd.opcode, \
                   cmd.parameters.data[0], cmd.parameters.data[1]);
            switch (cmd.opcode)
            {
                case HUDI_CEC_OPCODE_ACTIVE_SOURCE:
                    printf("\t OPCODE_ACTIVE_SOURCE\n");
                    break;
                case HUDI_CEC_OPCODE_SYSTEM_STANDBY:
                    printf("\t CEC_OPCODE_SYSTEM_STANDBY\n");
                    hdmirx_cec_dev_active_status_set(0);
                    enter_standby();
                    break;
                case HUDI_CEC_OPCODE_GIVE_POWER_STATUS:
                    printf("\t CEC_OPCODE_GIVE_POWER_STATUS\n");
                    _hdmirx_cec_report_power_stauts(cmd.initiator);
                    break;
                case OPCODE_MENU_STATUS:
                    printf("\t CEC_OPCODE_MENU_STATUS dummy\n");
                    break;
                case HUDI_CEC_OPCODE_VENDOR_COMMAND:
                    printf("\t CEC_OPCODE_VENDOR_COMMAND \n");
                    //cec_report_cur_output_stauts(cur_dev_la);
                    if (cmd.parameters.data[0] == 0x53)
                    {
                        if (cmd.parameters.data[1] == 1) // polling
                            printf("Vendor not ACK\n");
                        else if (cmd.parameters.data[1] == 0)
                            printf("Vendor ACK\n");
                    }
                    break;
                case HUDI_CEC_OPCODE_DEVICE_VENDOR_ID:
                    printf("\t CEC_OPCODE_DEVICE_VENDOR_ID dummy\n");
                    break;
                case HUDI_CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
                    printf("\t CEC_OPCODE_GIVE_DEVICE_VENDOR_ID dummy\n");
                    break;
                case HUDI_CEC_OPCODE_REPORT_PHYSICAL_ADDR:
                    printf("\t CEC_OPCODE_REPORT_PHYSICAL_ADDR dummy\n");
                    break;
                case HUDI_CEC_OPCODE_TEXT_VIEW_ON:
                    printf("\t CEC_OPCODE_TEXT_VIEW_ON dummy\n");
                    break;
                case HUDI_CEC_OPCODE_INACTIVE_SOURCE:
                    printf("\t CEC_OPCODE_INACTIVE_SOURCE dummy\n");
                    break;
                default:
                    printf("\t Feature Abort\n");
                    _hdmirx_cec_feature_abort(cmd.initiator);
                    break;
            }
        }

        if(hdmirx_get_plug_status() == HDMI_RX_STATUS_PLUGIN && hdmirx_cec_dev_active_status_get() == 0)
        {
            /* while cec device notified plugin and show , but it does not ready work with soc cec 
            *  connect when cec device first power on, so run cec procedure again.
            */
            if(tick_timeout < 60 * 100)
            {
                tick_count++;
                if(tick_count == 5 * 100)
                {
                    printf("%s %d \n", __func__, __LINE__);
                    hdmirx_cec_process_reload();
                    tick_timeout += tick_count ;  
                    tick_count = 0;
                }
            }
            
        }

        if(hdmirx_get_plug_status() == HDMI_RX_STATUS_PLUGOUT && tick_count != 0)
        {
            tick_count = 0;
        }
        usleep(10 * 1000);
    }
}

int hdmirx_cec_deinit()
{
    int ret = -1;
    if (g_hdmirx_cec_info.handle != NULL)
    {
        g_hdmirx_cec_info.polling_stop = 1;
        pthread_join(g_hdmirx_cec_info.pthread_id, NULL);
        app_log(LL_NOTICE,"%s   %d",__func__,__LINE__);
        ret = hudi_cec_close(g_hdmirx_cec_info.handle);
        g_hdmirx_cec_info.handle = NULL;
    }
    return ret;
}

int hdmirx_cec_init()
{
    pthread_attr_t attr;
    hudi_cec_config_t config = {};
    config.dev_path = "/dev/hdmi_rx";
    config.msgid_action = HUDI_CEC_MSGID_ACTION_CREATE;
    
    hudi_cec_open(&g_hdmirx_cec_info.handle, &config);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    if (pthread_create(&g_hdmirx_cec_info.pthread_id, &attr, hdmirx_cec_msg_task, g_hdmirx_cec_info.handle))
    {
        printf("create pthread fail %s,%d \n", __func__, __LINE__);
    }
    pthread_attr_destroy(&attr);
    return 0;
}
#endif
