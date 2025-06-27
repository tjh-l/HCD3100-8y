/**
 * @file
 * @brief                hudi cec module interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#include "hudi_cec.h"
#include <errno.h>
#include <stdint.h>
#include "hudi_cec_libcec.h"
#include "hudi_cec_types.h"
#include "hudi_cec_log.h"

int hudi_cec_open(hudi_handle *handle, const hudi_cec_config_t *config)
{
    if (handle == NULL || config == NULL)
        return -EINVAL;

    *handle = NULL;

    hudi_cec_libcec_t *libcec = (hudi_cec_libcec_t *)malloc(sizeof(hudi_cec_libcec_t));
    if (libcec == NULL)
    {
        return -ENOMEM;
    }
    memset(libcec, 0, sizeof(hudi_cec_libcec_t));

    libcec_configuration configuration = {};
    configuration.logical_addresses.primary =
        (cec_logical_address)config->logical_address;
    configuration.msg_id       = config->msg_id;
    configuration.msgid_action = (lib_cec_msgid_action_e)config->msgid_action;

    if (hudi_cec_initialise(libcec, &configuration))
    {
        goto open_fail;
    }
    *handle = libcec;
    hudi_cec(HUDI_LL_NOTICE, "opening a connection to the CEC adapter...\n");

    if (!libcec->iadapter->Open(libcec, config->dev_path, 10000))
    {
        hudi_cec(HUDI_LL_NOTICE, "unable to open the device on port %s\n", config->dev_path);
        goto open_fail;
    }

    return 0;

open_fail:

    if (libcec != NULL)
    {
        hudi_cec_destroy(libcec);
        free(libcec);
    }

    return -EFAULT;
}

int hudi_cec_close(hudi_handle handle)
{
    int ret = 0;
    if (handle == NULL)
    {
        return -EINVAL;
    }

    ret = hudi_cec_destroy(handle);
    if (ret == 0)
        free((hudi_cec_libcec_t *)handle);
    hudi_cec(HUDI_LL_NOTICE, "cec closed\n");
    return ret;
}

int hudi_cec_one_touch_play(hudi_handle handle)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->OneTouchPlay(libcec))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_standby_device(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->StandbyDevice(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_poweron_device(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->PowerOnDevice(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_scan_devices(hudi_handle                   handle,
                          hudi_cec_logical_addresses_t *laes)
{
    if (handle == NULL || laes == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    int ret = 0;

    ret = libcec->iadapter->ScanDevices(libcec, (void *)laes);

    return ret;
}

int hudi_cec_get_active_devices(hudi_handle                   handle,
                                hudi_cec_logical_addresses_t *laes,
                                const uint32_t                timewaitms)
{
    if (handle == NULL || laes == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    int ret = 0;

    ret = libcec->iadapter->GetActiveDevices(libcec, (void *)laes, timewaitms);

    return ret;
}

int hudi_cec_audio_volup(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->AudioVolUp(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_audio_voldown(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->AudioVolDown(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_audio_toggle_mute(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->AudioToggleMute(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_get_msgid(hudi_handle handle, int *msg_id)
{
    if (handle == NULL || msg_id  == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->GetMsgId(libcec, msg_id))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_msg_receive(hudi_handle handle, hudi_cec_cmd_t *cmd,
                         const bool nowait)
{
    if (handle == NULL || cmd == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->MsgRecv(libcec, cmd, nowait))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_key_press_through(hudi_handle handle, const hudi_cec_la_e la,
                               const hudi_cec_key_code_e key)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->KeyPressThrough(libcec, (cec_logical_address)la, (cec_user_control_code)key))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_get_device_power_status(hudi_handle handle, const hudi_cec_la_e la,
                                     hudi_cec_power_status_e *status,
                                     const int32_t            timeoutms)
{
    if (handle == NULL || status == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->GetDevicePowerStatus(libcec, (cec_logical_address)la,
                                                (cec_power_status *)status,
                                                timeoutms))
    {
        return -EFAULT;
    }

    return 0;
}

bool hudi_cec_is_active_source(hudi_handle handle, const hudi_cec_la_e la)
{
    if (handle == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->IsActiveSource(libcec, (cec_logical_address)la))
    {
        return -EFAULT;
    }

    return true;
}

int hudi_cec_get_device_vendor_id(hudi_handle handle, const hudi_cec_la_e la,
                                  uint32_t *vendorid, const int32_t timeoutms)
{
    if (handle == NULL || vendorid == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->GetDeviceVendorId(libcec, (cec_logical_address)la,
                                             vendorid, timeoutms))
    {
        return -EFAULT;
    }

    return 0;
}

int hudi_cec_send_special_command(hudi_handle handle, const hudi_cec_la_e la,
                                  const uint8_t                opcode,
                                  const hudi_cec_datapacket_t *params)
{
    if (handle == NULL || params == NULL)
        return -EINVAL;

    hudi_cec_libcec_t *libcec = handle;
    if (!libcec->iadapter->SendSpecialCmd(libcec, (cec_logical_address)la,
                                          (uint8_t)opcode, (void *)params))
    {
        return -EFAULT;
    }

    return 0;
}

//feature abort
