/**
 * @file
 * @brief                hudi cec module interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#include "hudi_cec_libcec.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hudi_cec_processor.h"
#include "hudi_cec_types.h"
#include "hudi_cec_log.h"

static int ref_obj_init(hudi_cec_libcec_ref_obj_t* thisptr)
{
    pthread_mutex_init(&thisptr->ref_mutex, NULL);
    thisptr->conn_status = HUDI_CEC_CONN_STATUS_FREE;
    thisptr->ref_count   = 0;

    return 0;
}

static int ref_obj_rls(hudi_cec_libcec_ref_obj_t* thisptr)
{
    pthread_mutex_destroy(&thisptr->ref_mutex);

    return 0;
}

static int ref_obj_ref(hudi_cec_libcec_ref_obj_t* thisptr)
{
    int ret = -1;
    pthread_mutex_lock(&thisptr->ref_mutex);
    if (thisptr->conn_status == HUDI_CEC_CONN_STATUS_FREE)
    {
        thisptr->ref_count++;
        ret = 0;
    }
    pthread_mutex_unlock(&thisptr->ref_mutex);

    return ret;
}

static int ref_obj_unref(hudi_cec_libcec_ref_obj_t* thisptr)
{
    pthread_mutex_lock(&thisptr->ref_mutex);
    thisptr->ref_count--;
    pthread_mutex_unlock(&thisptr->ref_mutex);

    return 0;
}

static int ref_obj_stop(hudi_cec_libcec_ref_obj_t* thisptr)
{
    int  loop_count = 0, ret = -1;
    bool loop_flag = true;
    pthread_mutex_lock(&thisptr->ref_mutex);
    thisptr->conn_status = HUDI_CEC_CONN_STATUS_CLOSING;
    pthread_mutex_unlock(&thisptr->ref_mutex);

    while (loop_count < 20 && loop_flag)
    {
        pthread_mutex_lock(&thisptr->ref_mutex);
        if (thisptr->ref_count == 0)
        {
            loop_flag = false;
            ret       = 0;
        }
        pthread_mutex_unlock(&thisptr->ref_mutex);
        if (loop_flag)
        {
            hudi_cec(HUDI_LL_NOTICE, "conn using, waiting\n");
            usleep(250 * 1000);
        }

    }

    return ret;
}

static bool icec_power_on_device(hudi_cec_libcec_t*                  thisptr,
                                 const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.PowerOn(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_audio_volup(hudi_cec_libcec_t*                  thisptr,
                                     const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.AudioVolUp(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_audio_voldown(hudi_cec_libcec_t*                  thisptr,
                                       const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.AudioVolDown(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_toggle_mute(hudi_cec_libcec_t*                  thisptr,
                                     const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.AudioVolMute(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_transmit(hudi_cec_libcec_t* thisptr, const cec_command* data)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.Transmit(thisptr->mcec, data, false);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;

}

static bool icec_adapter_standby_device(hudi_cec_libcec_t*                  thisptr,
                                        const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.StandbyDevice(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static int icec_adapter_scan_devices(hudi_cec_libcec_t*               thisptr,
                                     cec_logical_addresses* laes)
{
    int ret = -EFAULT;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.ScanDevices(thisptr->mcec, laes);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static int icec_adapter_get_active_devices(hudi_cec_libcec_t*               thisptr,
                                           cec_logical_addresses* laes,
                                           const uint32_t         timewaitms)
{
    int ret = -EFAULT;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.GetActiveDevices(thisptr->mcec, laes,
                                                        timewaitms);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_one_touch_play(hudi_cec_libcec_t* thisptr)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.OneTouchPlay(thisptr->mcec);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_set_logical_address(
    hudi_cec_libcec_t* thisptr, const cec_logical_address ilogical_address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.SetLogicalAddress(thisptr->mcec,
                                                         ilogical_address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_get_msgid(hudi_cec_libcec_t* thisptr, int* msg_id)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.GetMsgId(thisptr->mcec, msg_id);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_msg_recv(hudi_cec_libcec_t* thisptr, void* cmd,
                                  const bool nowait)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.MsgRecv(thisptr->mcec, cmd, nowait);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_keypress_through(hudi_cec_libcec_t*          thisptr,
                                          const cec_logical_address   address,
                                          const cec_user_control_code key)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.KeyPressThrough(thisptr->mcec, address,
                                                       key);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_get_device_power_status(hudi_cec_libcec_t* thisptr, const cec_logical_address address, cec_power_status* status, const int32_t timeoutms)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.GetDevicePowerStatus(
                  thisptr->mcec, address, status, timeoutms);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_is_active_source(hudi_cec_libcec_t*        thisptr,
                                          const cec_logical_address address)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.IsActiveSource(thisptr->mcec, address);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_get_device_vendorid(hudi_cec_libcec_t*        thisptr,
                                             const cec_logical_address address,
                                             uint32_t*                 vendorid,
                                             const int32_t             timeoutms)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.GetDeviceVendorId(thisptr->mcec, address,
                                                         vendorid, timeoutms);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_send_special_cmd(hudi_cec_libcec_t*        thisptr,
                                          const cec_logical_address address,
                                          const uint8_t opcode, const void* params)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.SendSpecialCmd(thisptr->mcec, address,
                                                      opcode, params);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static cec_logical_address icec_adapter_get_logical_address(hudi_cec_libcec_t* thisptr)
{
    bool ret = false;
    if (ref_obj_ref(&thisptr->ref_obj) == 0)
    {
        ret = thisptr->mcec->processor.GetLogicalAddress(thisptr->mcec);
        ref_obj_unref(&thisptr->ref_obj);
    }

    return ret;
}

static bool icec_adapter_open(hudi_cec_libcec_t* thisptr, const char* str_port,
                              uint32_t itimeoutms)
{
    if (!thisptr->mcec || !str_port)
        return false;

    // open a new connection
    if (!thisptr->mcec->processor.Start(
            thisptr->mcec, str_port, CEC_SERIAL_DEFAULT_BAUDRATE, itimeoutms))
    {
        hudi_cec(HUDI_LL_NOTICE, "could not start CEC communications\n");
        return false;
    }

    return true;
}

static void icec_adapter_close(hudi_cec_libcec_t* thisptr)
{
    if (thisptr->mcec)
    {
        thisptr->mcec->processor.Close(thisptr->mcec);
    }
}

int hudi_cec_destroy(void* conn)
{
    if (conn == NULL)
        return -EINVAL;

    hudi_cec_libcec_t* libcec = conn;

    if (libcec->mcec)
        libcec->mcec->processor.SetProcessorStatus(libcec->mcec, HUDI_CEC_PROCESSOR_STATUS_CLOSING);

    if (ref_obj_stop(&libcec->ref_obj) != 0)
        return -EBUSY;

    if (libcec->mcec)
    {
        libcec->iadapter->Close(libcec);
        hudi_cec_processor_destory(libcec->mcec);
        free(libcec->mcec);
    }

    if (libcec->iadapter)
    {
        free(libcec->iadapter);
    }

    ref_obj_rls(&libcec->ref_obj);

    return 0;
}

int hudi_cec_initialise(void* conn, const libcec_configuration* configuration)
{
    hudi_cec_libcec_t* libcec = conn;
    if (conn == NULL || !configuration)
    {
        hudi_cec(HUDI_LL_NOTICE, "init param error!\n");
        return -EINVAL;
    }

    memset(libcec, 0, sizeof(hudi_cec_libcec_t));
    ref_obj_init(&libcec->ref_obj);

    libcec->iadapter = (hudi_cec_iadapter_t*)malloc(sizeof(hudi_cec_iadapter_t));
    if (libcec->iadapter == NULL)
    {
        hudi_cec(HUDI_LL_NOTICE, "memory alloc failed!\n");
        return -EINVAL;
    }
    memset(libcec->iadapter, 0, sizeof(hudi_cec_iadapter_t));
    libcec->iadapter->Open              = icec_adapter_open;
    libcec->iadapter->Transmit          = icec_adapter_transmit;
    libcec->iadapter->Close             = icec_adapter_close;
    libcec->iadapter->PowerOnDevice     = icec_power_on_device;
    libcec->iadapter->SetLogicalAddress = icec_adapter_set_logical_address;
    libcec->iadapter->GetLogicalAddress = icec_adapter_get_logical_address;
    libcec->iadapter->StandbyDevice     = icec_adapter_standby_device;
    libcec->iadapter->OneTouchPlay      = icec_adapter_one_touch_play;
    libcec->iadapter->ScanDevices       = icec_adapter_scan_devices;
    libcec->iadapter->GetActiveDevices  = icec_adapter_get_active_devices;
    libcec->iadapter->AudioVolUp        = icec_adapter_audio_volup;
    libcec->iadapter->AudioVolDown      = icec_adapter_audio_voldown;
    libcec->iadapter->AudioToggleMute   = icec_adapter_toggle_mute;
    libcec->iadapter->GetMsgId          = icec_adapter_get_msgid;
    libcec->iadapter->MsgRecv           = icec_adapter_msg_recv;
    libcec->iadapter->KeyPressThrough   = icec_adapter_keypress_through;
    libcec->iadapter->GetDevicePowerStatus = icec_adapter_get_device_power_status;
    libcec->iadapter->IsActiveSource = icec_adapter_is_active_source;
    libcec->iadapter->GetDeviceVendorId = icec_adapter_get_device_vendorid;
    libcec->iadapter->SendSpecialCmd = icec_adapter_send_special_cmd;

    // init processor
    libcec->mcec = (hudi_cec_oprocessor_t*)malloc(sizeof(hudi_cec_oprocessor_t));
    if (libcec->mcec == NULL)
    {
        hudi_cec(HUDI_LL_NOTICE, "memory alloc failed!\n");
        goto cec_exit;
    }
    memset(libcec->mcec, 0, sizeof(hudi_cec_oprocessor_t));
    if (hudi_cec_processor_init(libcec->mcec, configuration) == false)
    {
        goto cec_exit;
    }
    return 0;

cec_exit:
    hudi_cec_destroy(conn);
    return 1;
}

