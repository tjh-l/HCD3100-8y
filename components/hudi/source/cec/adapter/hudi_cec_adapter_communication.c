#include "hudi_cec_adapter_communication.h"
#include <errno.h>
#include <fcntl.h>
#include <hcuapi/hdmi_cec.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../utils/hudi_cec_message.h"
#include "../utils/hudi_cec_threadutils.h"
#include "hudi_cec_types.h"
#include "hudi_cec_log.h"

#define INVALID_SOCKET_VALUE (-1)

// cec_command transfer to
static int cec_command_to_cec_info(const cec_command *   src,
                                   struct hdmi_cec_info *dst)
{
    dst->data[0]  = ((src->initiator << 4) | src->destination);
    dst->data[1]  = src->opcode;
    dst->data_len = src->parameters.size + 2;
    for (uint8_t i = 0; i < src->parameters.size; i++)
    {
        dst->data[i + 2] = src->parameters.data[i];
    }
    return 0;
}

static int cec_info_to_cec_command(const struct hdmi_cec_info *src,
                                   cec_command *               dst)
{
    dst->initiator       = src->data[0] >> 4;
    dst->destination     = src->data[0] & 0x0f;
    if (src->data_len >= 2)
    {
        dst->opcode          = src->data[1];
        dst->parameters.size = src->data_len - 2;
        for (uint8_t i = 0; i < dst->parameters.size; i++)
        {
            dst->parameters.data[i] = src->data[i + 2];
        }
    }

    return 0;
}

static bool IsOpen(hudi_cec_oadapter_communication_t *thisptr)
{
    return thisptr->mfd != INVALID_SOCKET_VALUE;
}

static void Close(hudi_cec_oadapter_communication_t *thisptr)
{
    hudi_cec_cthread_stop_thread(&thisptr->cthread, 0);
    close(thisptr->mfd);
    hudi_cec(HUDI_LL_DEBUG, "Close - mfd=%d\n", thisptr->mfd);
    thisptr->mfd = INVALID_SOCKET_VALUE;
}

static bool Open(hudi_cec_oadapter_communication_t *thisptr,
                 uint32_t itimeoutms, bool bskip_checks, bool bstart_listening)
{
    (void)bstart_listening;
    if (IsOpen(thisptr))
        Close(thisptr);

    if ((thisptr->mfd = open(thisptr->dev_path, O_RDWR)) < 0)
    {
        hudi_cec(HUDI_LL_ERROR, "open path fail:%s\n", thisptr->dev_path);
        return false;
    }

    if (!hudi_cec_cthread_create_thread(&thisptr->cthread, 1024 * 8, true))
    {
        hudi_cec(HUDI_LL_NOTICE, "adapter thread create fail\n");
        return false;
    }

    return true;
}

static hudi_cec_adapter_message_state_e Write(
    hudi_cec_oadapter_communication_t *thisptr, const cec_command *data,
    bool bretry, uint8_t iline_timeout, bool bis_reply)
{
    if (IsOpen(thisptr))
    {
        int                  ret      = -1;
        struct hdmi_cec_info cec_info = {};
        cec_command_to_cec_info(data, &cec_info);
        ret = ioctl(thisptr->mfd, HDMI_CEC_SEND_CMD, &cec_info);
        if (ret < 0)
        {
            hudi_cec(HUDI_LL_DEBUG, "transmit ret:%d\n\n", ret);
            return ADAPTER_MESSAGE_STATE_ERROR;
        }

    }

    return ADAPTER_MESSAGE_STATE_UNKNOWN;
}

static bool SetLogicalAddresses(hudi_cec_oadapter_communication_t *thisptr,
                                const cec_logical_addresses *      addresses)
{
    if (!IsOpen(thisptr))
        return false;

    if (ioctl(thisptr->mfd, HDMI_CEC_SET_LOGIC_ADDR, addresses->primary))
    {
        hudi_cec(HUDI_LL_ERROR,
                 "CRTOSCECAdapterCommunication::SetLogicalAddresses - ioctl "
                 "CEC_ADAP_S_LOG_ADDRS failed - errno=%d\n",
                 errno);
        return false;
    }

    return true;
}

static int GetLogicalAddresses(hudi_cec_oadapter_communication_t *thisptr,
                               cec_logical_address *              address)
{
    if (!IsOpen(thisptr))
        return -EFAULT;

    if (ioctl(thisptr->mfd, HDMI_CEC_GET_LOGIC_ADDR, &address))
    {
        hudi_cec(HUDI_LL_NOTICE, "get LA failed!\n");
        return -EFAULT;
    }
    hudi_cec(HUDI_LL_NOTICE, "LA:%d\n", *address);

    return 0;
}

static int GetPhysicalAddress(hudi_cec_oadapter_communication_t *thisptr,
                              unsigned int *physical_address)
{
    if (!IsOpen(thisptr))
        return -EFAULT;

    if (ioctl(thisptr->mfd, HDMI_CEC_GET_PHYSICAL_ADDR, physical_address))
    {
        hudi_cec(HUDI_LL_NOTICE, "get PA failed!\n");
        return -EFAULT;
    }

    return 0;
}

static cec_vendor_id GetVendorId(hudi_cec_oadapter_communication_t *thisptr)
{
    if (IsOpen(thisptr))
    {
    }

    return CEC_VENDOR_UNKNOWN;
}

static void *Process(void *thisobj)
{
    hudi_cec_oadapter_communication_t *comm =
        (hudi_cec_oadapter_communication_t *)thisobj;

    while (!hudi_cec_cthread_is_stopped(&comm->cthread))
    {
        if (comm->mfd >= 0)
        {
            hdmi_cec_info_t cec_info = {};
            if (ioctl(comm->mfd, HDMI_CEC_GET_CMD, &cec_info) == 0)
            {
                hudi_cec(HUDI_LL_DEBUG, "GET CEC CMD\n");
                for (unsigned int i = 0; i < cec_info.data_len; i++)
                {
                    hudi_cec_pr(HUDI_LL_DEBUG, "0x%x ", cec_info.data[i]);
                }
                hudi_cec_pr(HUDI_LL_DEBUG, "\n");

                // parse info
                cec_command cmd = {};
                cec_info_to_cec_command(&cec_info, &cmd);

                pthread_mutex_lock(&comm->msg_obj->mutex);
                hudi_cec_api_message_send(comm->msg_obj->msg_handle, &cmd,
                                          sizeof(cec_command));
                pthread_cond_signal(&comm->msg_obj->cond);
                pthread_mutex_unlock(&comm->msg_obj->mutex);
            }
        }
        if (!hudi_cec_cthread_is_stopped(&comm->cthread))
            usleep(1000 * 10);
    }

    hudi_cec(HUDI_LL_NOTICE, "rtos adapter - stopped\n");

    return NULL;
}

static int initialise_api(hudi_cec_oadapter_communication_t *comm,
                          hudi_cec_cthread_msg_t *           msg_obj)
{
    memset(comm, 0, sizeof(hudi_cec_oadapter_communication_t));
    // todo operation
    comm->mcommunication.Open                = Open;
    comm->mcommunication.IsOpen              = IsOpen;
    comm->mcommunication.Close               = Close;
    comm->mcommunication.Write               = Write;
    comm->mcommunication.SetLogicalAddresses = SetLogicalAddresses;
    comm->mcommunication.GetLogicalAddresses = GetLogicalAddresses;
    comm->mcommunication.GetPhysicalAddress = GetPhysicalAddress;

    // fd init
    comm->mfd    = INVALID_SOCKET_VALUE;
    comm->msg_obj = msg_obj;
    hudi_cec_cthread_thread_init(&comm->cthread, (void *)comm, Process);
    return 0;
}
#if 0
static bool SetLineTimeout(uint8_t iTimeout)
{
    return true;
}
static bool StartBootloader(void)
{
    return false;
}
static bool PingAdapter(void)
{
    return true;
}
static uint16_t GetFirmwareVersion(void)
{
    return 0;
}
static uint32_t GetFirmwareBuildDate(void)
{
    return 0;
}
static bool IsRunningLatestFirmware(void)
{
    return true;
}
static bool SetControlledMode(bool controlled)
{
    return true;
}
static bool SaveConfiguration(const libcec_configuration *configuration)
{
    return false;
}
static bool SetAutoMode(bool automode)
{
    return false;
}
static bool GetConfiguration(libcec_configuration *configuration)
{
    return false;
}
static char *GetPortName(void)
{
    return NULL;
}
static bool SupportsSourceLogicalAddress(const cec_logical_address address)
{
    return address > CECDEVICE_TV && address <= CECDEVICE_BROADCAST;
}
static cec_adapter_type GetAdapterType(void)
{
    return ADAPTERTYPE_UNKNOWN;
}
static uint16_t GetAdapterVendorId(void)
{
    return 1;
}
static uint16_t GetAdapterProductId(void)
{
    return 1;
}
static void SetActiveSource(bool bSetTo, bool bClientUnregistered)
{
}
#endif

int hudi_cec_get_instance(hudi_cec_oadapter_communication_t **comm,
                          hudi_cec_cthread_msg_t *msg_obj, const char *str_port,
                          uint16_t ibaud_rate)
{
    (void)ibaud_rate;
    int ret = 1;
    *comm   = (hudi_cec_oadapter_communication_t *)malloc(
                  sizeof(hudi_cec_oadapter_communication_t));
    if (*comm == NULL)
    {
        hudi_cec(HUDI_LL_NOTICE, "mem alloc failed\n");
        return 1;
    }
    memset(*comm, 0, sizeof(hudi_cec_oadapter_communication_t));
    ret               = initialise_api(*comm, msg_obj);
    (*comm)->dev_path = (char *)str_port;

    return ret;
}

void hudi_cec_destroy_instance(hudi_cec_oadapter_communication_t **comm)
{
    if (*comm != NULL)
    {
        free(*comm);
    }
}