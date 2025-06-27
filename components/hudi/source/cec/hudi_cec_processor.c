#include "hudi_cec_processor.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "hudi_cec_types.h"
#include "hudi_cec.h"
#include "string.h"
#include "utils/hudi_cec_message.h"
#include "hudi_cec_log.h"

#define CTL_MSG_COUNT 101

static void cec_datapacket_pushback(cec_datapacket *thisptr, uint8_t add)
{
    if (thisptr->size < CEC_MAX_DATA_PACKET_SIZE)
        thisptr->data[thisptr->size++] = add;
}

/*!
 * @brief Clear this packet.
 */
static void cec_datapacket_clear(cec_datapacket *thisptr)
{
    memset(thisptr->data, 0, sizeof(thisptr->data));
    thisptr->size = 0;
}

static void cec_datapacket_assignment(cec_datapacket *thisptr, const cec_datapacket *src)
{
    cec_datapacket_clear(thisptr);
    for (uint8_t iPtr = 0; iPtr < src->size; iPtr++)
        cec_datapacket_pushback(thisptr, src->data[iPtr]);
}

static bool cec_datapacket_equal(const cec_datapacket *thisptr, const cec_datapacket *packet)
{
    if (thisptr->size != packet->size)
        return false;

    for (uint8_t iPtr = 0; iPtr < thisptr->size; iPtr++)
        if (packet->data[iPtr] != thisptr->data[iPtr])
            return false;

    return true;
}

/** @return True when this packet is empty, false otherwise. */
static bool cec_datapacket_iempty(cec_datapacket *thisptr)
{
    return thisptr->size == 0;
}

/** @return True when this packet is false, false otherwise. */
static bool cec_datapacket_ifull(cec_datapacket *thisptr)
{
    return thisptr->size == CEC_MAX_DATA_PACKET_SIZE;
}

/*!
 * @brief Shift the contents of this packet.
 * @param iShiftBy The number of positions to shift.
 */
static void cec_datapacket_shift(cec_datapacket *thisptr, uint8_t iShiftBy)
{
    if (iShiftBy >= thisptr->size)
    {
        cec_datapacket_clear(thisptr);
    }
    else
    {
        for (uint8_t iPtr = 0; iPtr < thisptr->size; iPtr++)
            thisptr->data[iPtr] = (iPtr + iShiftBy < thisptr->size) ? thisptr->data[iPtr + iShiftBy] : 0;
        thisptr->size = (uint8_t)(thisptr->size - iShiftBy);
    }
}

// cec command api
/*!
 * @brief Clear this command, resetting everything to the default values.
 */
static void cec_command_clear(cec_command *thisptr)
{
    thisptr->initiator = CECDEVICE_UNKNOWN;
    thisptr->destination = CECDEVICE_UNKNOWN;
    thisptr->ack = 0;
    thisptr->eom = 0;
    thisptr->opcode_set = 0;
    thisptr->opcode = CEC_OPCODE_FEATURE_ABORT;
    thisptr->transmit_timeout = CEC_DEFAULT_TRANSMIT_TIMEOUT;
    cec_datapacket_clear(&thisptr->parameters);
};

static void cec_command_assignment(cec_command *dst, const cec_command *src)
{
    dst->initiator = src->initiator;
    dst->destination = src->destination;
    dst->ack = src->ack;
    dst->eom = src->eom;
    dst->opcode = src->opcode;
    dst->opcode_set = src->opcode_set;
    dst->transmit_timeout = src->transmit_timeout;
    dst->parameters = src->parameters;
}

/*!
 * @brief Formats a cec_command.
 * @param command The command to format.
 * @param initiator The logical address of the initiator.
 * @param destination The logical address of the destination.
 * @param opcode The opcode of the command.
 * @param timeout The transmission timeout.
 */
static void cec_command_format(cec_command *command, cec_logical_address initiator, cec_logical_address destination,
                               cec_opcode opcode, int32_t timeout)
{
    cec_command_clear(command);
    command->initiator = initiator;
    command->destination = destination;
    command->transmit_timeout = timeout;
    if (opcode != CEC_OPCODE_NONE)
    {
        command->opcode = opcode;
        command->opcode_set = 1;
    }
}

/*!
 * @brief Push a byte to the back of this command.
 * @param data The byte to push.
 */
static void cec_command_pushback(cec_command *thisptr, uint8_t data)
{
    if (thisptr->initiator == CECDEVICE_UNKNOWN && thisptr->destination == CECDEVICE_UNKNOWN)
    {
        thisptr->initiator = (cec_logical_address)(data >> 4);
        thisptr->destination = (cec_logical_address)(data & 0xF);
    }
    else if (!thisptr->opcode_set)
    {
        thisptr->opcode_set = 1;
        thisptr->opcode = (cec_opcode)data;
    }
    else
        cec_datapacket_pushback(&thisptr->parameters, data);
}

static cec_opcode GetResponseOpcode(cec_opcode opcode)
{
    switch (opcode)
    {
        case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
            return CEC_OPCODE_ACTIVE_SOURCE;
        case CEC_OPCODE_GET_CEC_VERSION:
            return CEC_OPCODE_CEC_VERSION;
        case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
            return CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
        case CEC_OPCODE_GET_MENU_LANGUAGE:
            return CEC_OPCODE_SET_MENU_LANGUAGE;
        case CEC_OPCODE_GIVE_DECK_STATUS:
            return CEC_OPCODE_DECK_STATUS;
        case CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS:
            return CEC_OPCODE_TUNER_DEVICE_STATUS;
        case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
            return CEC_OPCODE_DEVICE_VENDOR_ID;
        case CEC_OPCODE_GIVE_OSD_NAME:
            return CEC_OPCODE_SET_OSD_NAME;
        case CEC_OPCODE_MENU_REQUEST:
            return CEC_OPCODE_MENU_STATUS;
        case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
            return CEC_OPCODE_REPORT_POWER_STATUS;
        case CEC_OPCODE_GIVE_AUDIO_STATUS:
            return CEC_OPCODE_REPORT_AUDIO_STATUS;
        case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
            return CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS;
        case CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
            return CEC_OPCODE_SET_SYSTEM_AUDIO_MODE;
        default:
            break;
    }

    return CEC_OPCODE_NONE;
}

static void cec_command_push_array(cec_command *thisptr, size_t len, const uint8_t *data)
{
    for (size_t iPtr = 0; iPtr < len; iPtr++)
        cec_command_pushback(thisptr, data[iPtr]);
}

static int check_devices_bit_exist(cec_logical_addresses *device_list,
                                   int bit_offset)
{
    int temp = 0;
    for (int i = 0; i < 15; i++)
    {
        temp |= device_list->addresses[i] & (1 << bit_offset);
    }
    return temp > 0 ? 0 : -ENODEV;
}

// device
/*!
 * @brief Add a type to this list.
 * @param type The type to add.
 */
static void device_type_list_add(cec_device_type_list *device_type_list, const cec_device_type type)
{
    for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
    {
        if (device_type_list->types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
        {
            device_type_list->types[iPtr] = type;
            break;
        }
    }
}

/**
 * @brief cmd buff
 *
 */
static int hudi_cec_command_free_buff_id_get(hudi_cec_oprocessor_t *thisptr)
{
    int ret = -1;
    pthread_mutex_lock(&thisptr->cmd_buf_mutex);
    for (int i = 0; i < HUDI_CEC_COMMAND_BUFF_COUNT; i++)
    {
        if (thisptr->cmd[i].status == HUDI_CEC_COMMAND_BUFF_STATUS_FREE)
        {
            thisptr->cmd[i].status = HUDI_CEC_COMMAND_BUFF_STATUS_USING;
            pthread_mutex_init(&thisptr->cmd[i].mutex, NULL);
            pthread_cond_init(&thisptr->cmd[i].cond, NULL);
            ret = i;
            break;
        }
    }
    pthread_mutex_unlock(&thisptr->cmd_buf_mutex);

    return ret;
}

static int hudi_cec_command_free_buff(hudi_cec_oprocessor_t *thisptr, int id)
{
    pthread_mutex_destroy(&thisptr->cmd[id].mutex);
    pthread_cond_destroy(&thisptr->cmd[id].cond);

    pthread_mutex_lock(&thisptr->cmd_buf_mutex);
    thisptr->cmd[id].status = HUDI_CEC_COMMAND_BUFF_STATUS_FREE;
    pthread_mutex_unlock(&thisptr->cmd_buf_mutex);

    return 0;
}

/**
 * @brief processor api
 *
 */
static bool processor_keyrelease(hudi_cec_oprocessor_t *thisptr, const cec_logical_address initiator, const cec_logical_address destination, bool bWait)
{
    (void)bWait;
    cec_command cmd = {};
    cec_command_format(&cmd, initiator, destination, CEC_OPCODE_USER_CONTROL_RELEASE, 0);

    return thisptr->processor.Transmit(thisptr, &cmd, false);
}

static bool processor_keypress(hudi_cec_oprocessor_t *thisptr, const cec_logical_address initiator,
                               const cec_logical_address destination, cec_user_control_code key, bool bWait)
{
    (void)bWait;
    cec_command cmd = {};
    cec_command_format(&cmd, initiator, destination, CEC_OPCODE_USER_CONTROL_PRESSED, 0);
    cec_datapacket_pushback(&cmd.parameters, (uint8_t)key);

    return thisptr->processor.Transmit(thisptr, &cmd, false);
}

static bool processor_keypress_through(hudi_cec_oprocessor_t *   thisptr, const cec_logical_address destination, cec_user_control_code key)
{
    return thisptr->processor.KeyPress(thisptr, thisptr->device_list.primary,
                                       destination, key, false) &&
           thisptr->processor.KeyRelease(thisptr, thisptr->device_list.primary,
                                         destination, false);
}

static bool processor_get_device_power_status(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination,
                                              cec_power_status *status, const int32_t timeoutms)
{
    int cmd_buf_id = hudi_cec_command_free_buff_id_get(thisptr);
    if (cmd_buf_id < 0)
        return false;

    cec_command_format(&thisptr->cmd[cmd_buf_id].cmd,
                       thisptr->device_list.primary, destination,
                       CEC_OPCODE_GIVE_DEVICE_POWER_STATUS, timeoutms);
    pthread_mutex_lock(&thisptr->cmd_buf_mutex);
    thisptr->cmd[cmd_buf_id].status = HUDI_CEC_COMMAND_BUFF_STATUS_WAITING_RES;
    pthread_mutex_unlock(&thisptr->cmd_buf_mutex);

    bool ret = thisptr->processor.Transmit(thisptr,
                                           &thisptr->cmd[cmd_buf_id].cmd, true);
    if (!ret)
        goto hudi_cec_command_free_buff;

    struct timespec ts;
    int wait_ret = 0;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (timeoutms % 1000) * 1000 * 1000;
    ts.tv_sec += timeoutms / 1000; // ms ->s
    //wait
    pthread_mutex_lock(&thisptr->cmd[cmd_buf_id].mutex);
    wait_ret = pthread_cond_timedwait(&thisptr->cmd[cmd_buf_id].cond,
                                      &thisptr->cmd[cmd_buf_id].mutex, &ts);
    pthread_mutex_unlock(&thisptr->cmd[cmd_buf_id].mutex);
    if (wait_ret != 0)
    {
        ret = false;
        goto hudi_cec_command_free_buff;
    }

    //get status
    hudi_cec_device_info_t device;
    pthread_mutex_lock(&thisptr->dev_list_mutex);
    device.status = thisptr->devices[destination].status;
    pthread_mutex_unlock(&thisptr->dev_list_mutex);
    *status = device.status;

hudi_cec_command_free_buff:
    hudi_cec_command_free_buff(thisptr, cmd_buf_id);
    return ret;
}

static bool processor_standby(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination)
{
    cec_command cmd = {};
    cec_command_format(&cmd, thisptr->device_list.primary, destination, CEC_OPCODE_STANDBY, 0);

    return thisptr->processor.Transmit(thisptr, &cmd, false);
}

static bool processor_one_touch_play(hudi_cec_oprocessor_t *thisptr)
{
    cec_command cmd = {};
    cec_command_format(&cmd, thisptr->device_list.primary, CECDEVICE_TV, CEC_OPCODE_IMAGE_VIEW_ON, 0);

    if (!thisptr->processor.Transmit(thisptr, &cmd, false))
    {
        return false;
    }
    unsigned int physical_address = 0xffffffff;
    cec_command_format(&cmd, thisptr->device_list.primary, CECDEVICE_TV, CEC_OPCODE_ACTIVE_SOURCE, 0);
    if (!thisptr->processor.GetPhysicalAddress(thisptr, &physical_address))
        return false;
    cec_datapacket_pushback(&cmd.parameters, (uint8_t)(physical_address >> 8) & 0xff);
    cec_datapacket_pushback(&cmd.parameters, (uint8_t)(physical_address) & 0xff);


    return thisptr->processor.Transmit(thisptr, &cmd, false);
}

static int processor_get_active_devices(hudi_cec_oprocessor_t *thisptr, cec_logical_addresses *laes,
                                        const uint32_t timewaitms)
{
    int ret = 0;
    cec_command cmd = {};
    cec_command_format(&cmd, thisptr->device_list.primary, CECDEVICE_BROADCAST, CEC_OPCODE_REQUEST_ACTIVE_SOURCE,
                       0);
    if (!thisptr->processor.Transmit(thisptr, &cmd, false))
        return -ECOMM;

    usleep(1000 * timewaitms);

    // memcpy
    pthread_mutex_lock(&thisptr->dev_list_mutex);
    if (check_devices_bit_exist(&thisptr->device_list, 1) != 0)
    {
        ret = -ENODEV;
    }
    else
    {
        memcpy(laes, &thisptr->device_list, sizeof(cec_logical_addresses));
    }
    pthread_mutex_unlock(&thisptr->dev_list_mutex);

    return 0;
}

static int processor_scan_devices(hudi_cec_oprocessor_t *thisptr, cec_logical_addresses *laes)
{
    int ret = -ENODEV;
    cec_command cmd = {};

    for (cec_logical_address i = CECDEVICE_TV; i < CECDEVICE_BROADCAST; i++)
    {
        if (i != thisptr->device_list.primary)
        {
            if (thisptr->processor.CheckProcessorStatus(thisptr, HUDI_CEC_PROCESSOR_STATUS_CLOSING))
            {
                ret = -EACCES;
                break;
            }
            cec_command_format(&cmd, thisptr->device_list.primary, i, CEC_OPCODE_NONE, 0);
            if (thisptr->processor.Transmit(thisptr, &cmd, false))
            {
                ret = 0;
                pthread_mutex_lock(&thisptr->dev_list_mutex);
                if (thisptr->device_list.addresses[i] == 0)
                {
                    thisptr->device_list.addresses[i] = HUDI_CEC_DEVICE_PING;
                }
                pthread_mutex_unlock(&thisptr->dev_list_mutex);
            }
        }
    }

    // memcpy
    pthread_mutex_lock(&thisptr->dev_list_mutex);
    if (ret == 0)
    {
        memcpy(laes, &thisptr->device_list, sizeof(cec_logical_addresses));
    }
    else
    {
        memset(thisptr->device_list.addresses, 0, 16 * sizeof(int));
    }
    pthread_mutex_unlock(&thisptr->dev_list_mutex);

    return ret;
}

static bool processor_poweron(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination)
{
    return thisptr->processor.KeyPress(thisptr, thisptr->device_list.primary,
                                       destination,
                                       CEC_USER_CONTROL_CODE_POWER, false) &&
           thisptr->processor.KeyRelease(thisptr, thisptr->device_list.primary,
                                         destination, false);
}

static bool processor_audio_volup(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination)
{
    return thisptr->processor.KeyPress(
               thisptr, thisptr->device_list.primary, destination,
               CEC_USER_CONTROL_CODE_VOLUME_UP, false) &&
           thisptr->processor.KeyRelease(thisptr, thisptr->device_list.primary,
                                         destination, false);
}

static bool processor_audio_voldown(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination)
{
    return thisptr->processor.KeyPress(
               thisptr, thisptr->device_list.primary, destination,
               CEC_USER_CONTROL_CODE_VOLUME_DOWN, false) &&
           thisptr->processor.KeyRelease(thisptr, thisptr->device_list.primary,
                                         destination, false);
}

static bool processor_audio_toggle_mute(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination)
{
    return thisptr->processor.KeyPress(thisptr, thisptr->device_list.primary,
                                       destination, CEC_USER_CONTROL_CODE_MUTE,
                                       false) &&
           thisptr->processor.KeyRelease(thisptr, thisptr->device_list.primary,
                                         destination, false);
}

static void processor_mark_busy(hudi_cec_oprocessor_t *thisptr)
{
    pthread_mutex_lock(&thisptr->adapter_mutex);
    return;
}

static void processor_mark_ready(hudi_cec_oprocessor_t *thisptr)
{
    pthread_mutex_unlock(&thisptr->adapter_mutex);
    return;
}

static bool processor_transmit(hudi_cec_oprocessor_t *thisptr, const cec_command *data, bool bneed_reply)
{
    cec_command transmitData;
    cec_command_assignment(&transmitData, data);

    if (data->initiator == CECDEVICE_UNKNOWN && data->destination == CECDEVICE_UNKNOWN)
        return false;

    if (!thisptr->mcomm)
        return false;

    hudi_cec(HUDI_LL_DEBUG, "sending - cmd:(%d,%d,0x%x", transmitData.initiator, transmitData.destination,
             transmitData.opcode);
    for (uint8_t i = 0; i < transmitData.parameters.size; i++)
    {
        hudi_cec_pr(HUDI_LL_DEBUG, ",0x%x", transmitData.parameters.data[i]);
    }
    hudi_cec_pr(HUDI_LL_DEBUG, ")\n");

    hudi_cec_adapter_message_state_e ret = ADAPTER_MESSAGE_STATE_UNKNOWN;
    thisptr->processor.MarkBusy(thisptr);
    ret = thisptr->mcomm->mcommunication.Write(
              thisptr->mcomm, &transmitData, true, (uint8_t)1000, false);
    thisptr->processor.MarkReady(thisptr);
    if (ret != ADAPTER_MESSAGE_STATE_UNKNOWN)
    {
        return false;
    }

    return true;
}

static bool processor_set_logical_address(hudi_cec_oprocessor_t *thisptr, const cec_logical_address address)
{
    cec_logical_addresses addresses = {};
    addresses.primary = address;
    bool ret = true;

    thisptr->processor.MarkBusy(thisptr);
    ret = thisptr->mcomm->mcommunication.SetLogicalAddresses(thisptr->mcomm, &addresses);
    if (ret == true)
        thisptr->device_list.primary = address;
    thisptr->processor.MarkReady(thisptr);

    return ret;
}

static cec_logical_address processor_get_logical_address(hudi_cec_oprocessor_t *thisptr)
{
    cec_logical_address address = CECDEVICE_UNREGISTERED;
    int                 ret     = 0;
    thisptr->processor.MarkBusy(thisptr);
    ret = thisptr->mcomm->mcommunication.GetLogicalAddresses(
              thisptr->mcomm, &address);
    if (ret == -EFAULT)
        address = thisptr->device_list.primary;
    thisptr->processor.MarkReady(thisptr);

    return address;
}

static void processor_reset_members(hudi_cec_oprocessor_t *thisptr)
{
    (void)thisptr;
}

static void processor_set_cec_initialised(hudi_cec_oprocessor_t *thisptr, bool bset_to /* = true */)
{
    thisptr->binitialised = bset_to;
}

static bool processor_open_connection(hudi_cec_oprocessor_t *thisptr, const char *str_port, uint16_t ibaud_rate,
                                      uint32_t itimeoutms, bool bstart_listening /* = true */)
{
    (void)itimeoutms;
    (void)bstart_listening;

    // ensure that a previous connection is closed
    if (thisptr->mcomm)
        thisptr->processor.Close(thisptr);

    // reset all member to the initial state
    thisptr->processor.ResetMembers(thisptr);

    // check whether the Close() method deleted any previous connection
    if (thisptr->mcomm)
    {
        hudi_cec(HUDI_LL_NOTICE, "previous connection could not be closed\n");
        return false;
    }

    // create a new connection
    if (hudi_cec_get_instance(&thisptr->mcomm, &thisptr->msg_obj, str_port, ibaud_rate))
    {
        hudi_cec(HUDI_LL_NOTICE, "get instance failed\n");
        return false;
    }

    if (!thisptr->mcomm->mcommunication.Open(
            thisptr->mcomm, false, false, true))
        return false;
    if (!thisptr->mcomm->mcommunication.SetLogicalAddresses(thisptr->mcomm, &thisptr->device_list))
        return false;
    hudi_cec(HUDI_LL_NOTICE, "connection opened\n");

    // mark as initialised
    thisptr->processor.SetCECInitialised(thisptr, true);

    return true;
}

static bool processsor_start(hudi_cec_oprocessor_t *thisptr, const char *str_port, uint16_t ibaud_rate,
                             uint32_t itimeoutms)
{
    // open a connection
    if (!thisptr->processor.OpenConnection(thisptr, str_port, ibaud_rate, itimeoutms, true))
        return false;

    // create the processor thread
    if (!hudi_cec_cthread_create_thread(&thisptr->cthread, 1024 * 8, true))
    {
        hudi_cec(HUDI_LL_NOTICE, "create a processor thread fail\n");
        return false;
    }

    return true;
}

static bool processor_get_msgid(hudi_cec_oprocessor_t *thisptr, int *msg_id)
{
    if (!(thisptr->msgid_action != LIB_CEC_MSGID_ACTION_NONE && thisptr->msg_id > 0))
    {
        return false;
    }

    *msg_id = thisptr->msg_id;

    return true;
}

static bool processor_msg_recv(hudi_cec_oprocessor_t *thisptr, void *cmd, bool nowait)
{
    if (cmd == NULL || !(thisptr->msgid_action != LIB_CEC_MSGID_ACTION_NONE && thisptr->msg_id > 0))
    {
        return false;
    }
    hudi_cec_cmd_t *cmdt = cmd;
    if (hudi_cec_api_message_recv(thisptr->msg_id, cmdt, sizeof(hudi_cec_cmd_t),
                                  nowait) != 0)
        return false;

    return true;
}

static bool processor_is_active_source(hudi_cec_oprocessor_t *   thisptr,
                                       const cec_logical_address destination)
{
    bool ret = false;
    if (destination > 15)
        return false;

    pthread_mutex_lock(&thisptr->dev_list_mutex);
    if (thisptr->device_list.addresses[destination] > 1)
    {
        ret = true;
    }
    pthread_mutex_unlock(&thisptr->dev_list_mutex);

    return ret;
}

static bool processor_get_device_vendor_id(hudi_cec_oprocessor_t *   thisptr,
                                           const cec_logical_address destination,
                                           uint32_t *                vendorid,
                                           const int32_t             timeoutms)
{
    int cmd_buf_id = hudi_cec_command_free_buff_id_get(thisptr);
    if (cmd_buf_id < 0)
        return false;

    cec_command_format(&thisptr->cmd[cmd_buf_id].cmd,
                       thisptr->device_list.primary, destination,
                       CEC_OPCODE_GIVE_DEVICE_VENDOR_ID, timeoutms);
    pthread_mutex_lock(&thisptr->cmd_buf_mutex);
    thisptr->cmd[cmd_buf_id].status = HUDI_CEC_COMMAND_BUFF_STATUS_WAITING_RES;
    pthread_mutex_unlock(&thisptr->cmd_buf_mutex);

    bool ret = thisptr->processor.Transmit(thisptr,
                                           &thisptr->cmd[cmd_buf_id].cmd, true);
    if (!ret)
        goto hudi_cec_command_free_buff;

    struct timespec ts;
    int wait_ret = 0;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (timeoutms % 1000) * 1000 * 1000;
    ts.tv_sec += timeoutms / 1000; //ms ->s
    //wait
    pthread_mutex_lock(&thisptr->cmd[cmd_buf_id].mutex);
    wait_ret = pthread_cond_timedwait(&thisptr->cmd[cmd_buf_id].cond,
                                      &thisptr->cmd[cmd_buf_id].mutex, &ts);
    pthread_mutex_unlock(&thisptr->cmd[cmd_buf_id].mutex);
    if (wait_ret != 0)
        ret = false;

    //get status
    hudi_cec_device_info_t device;
    pthread_mutex_lock(&thisptr->dev_list_mutex);
    device.vendorid = thisptr->devices[destination].vendorid;
    pthread_mutex_unlock(&thisptr->dev_list_mutex);
    *vendorid = device.vendorid;

hudi_cec_command_free_buff:
    hudi_cec_command_free_buff(thisptr, cmd_buf_id);
    return ret;
}

static bool processor_send_special_cmd(hudi_cec_oprocessor_t *   thisptr,
                                       const cec_logical_address destination,
                                       const uint8_t opcode, const void *params)
{
    cec_datapacket *parameters = (cec_datapacket *)params;
    cec_command     cmd        = {};
    cec_command_format(&cmd, thisptr->device_list.primary, destination,
                       (cec_opcode)opcode, 0);
    for (int i = 0; i < parameters->size; i++)
    {
        cec_datapacket_pushback(&cmd.parameters, parameters->data[i]);
    }

    bool ret = thisptr->processor.Transmit(thisptr, &cmd, false);

    return ret;
}

static bool processor_get_physical_address(hudi_cec_oprocessor_t *thisptr,
                                           unsigned int *physical_address)
{
    int ret = true;
    thisptr->processor.MarkBusy(thisptr);
    if (thisptr->mcomm->mcommunication.GetPhysicalAddress(
            thisptr->mcomm, physical_address) != 0)
        ret = false;
    thisptr->processor.MarkReady(thisptr);
    return true;
}

static int processor_set_proc_status(hudi_cec_oprocessor_t *     thisptr,
                                     hudi_cec_processor_status_e status)
{
    thisptr->proc_status = status;
    return 0;
}

static bool processor_check_proc_status(hudi_cec_oprocessor_t *     thisptr,
                                        hudi_cec_processor_status_e status)
{
    return thisptr->proc_status == status;
}

static int hudi_cec_process_cmd_res(hudi_cec_oprocessor_t *thisptr,
                                    cec_command *          cmd)
{
    if (cmd->opcode == CEC_OPCODE_REPORT_POWER_STATUS)
    {
        pthread_mutex_lock(&thisptr->dev_list_mutex);
        thisptr->devices[cmd->initiator].status = cmd->parameters.data[0];
        hudi_cec(HUDI_LL_DEBUG, "hudi_cec opcode 8f:%d\n",
                 thisptr->devices[cmd->initiator].status);
        pthread_mutex_unlock(&thisptr->dev_list_mutex);
    }
    else if (cmd->opcode == CEC_OPCODE_DEVICE_VENDOR_ID)
    {
        uint32_t vendorid = ((uint32_t)cmd->parameters.data[0] << 16) |
                            ((uint32_t)cmd->parameters.data[1] << 8) |
                            ((uint32_t)cmd->parameters.data[2]);
        pthread_mutex_lock(&thisptr->dev_list_mutex);
        thisptr->devices[cmd->initiator].vendorid = vendorid;
        hudi_cec(HUDI_LL_DEBUG, "hudi_cec opcode 87:%x\n",
                 thisptr->devices[cmd->initiator].vendorid);
        pthread_mutex_unlock(&thisptr->dev_list_mutex);
    }
    else
    {
        return -1;
    }
    return 0;
}

static int hudi_cec_handle_cmd(hudi_cec_oprocessor_t *thisptr, cec_command *cmd)
{
    int ret = -1;
    for (int i = 0; i < HUDI_CEC_COMMAND_BUFF_COUNT; i++)
    {
        pthread_mutex_lock(&thisptr->cmd_buf_mutex);
        if (thisptr->cmd[i].status ==
            HUDI_CEC_COMMAND_BUFF_STATUS_WAITING_RES)
        {
            if (GetResponseOpcode(thisptr->cmd[i].cmd.opcode) == cmd->opcode &&
                (thisptr->cmd[i].cmd.initiator == cmd->destination ||
                 cmd->destination == HUDI_CEC_DEVICE_BROADCAST))
            {
                ret = hudi_cec_process_cmd_res(thisptr, cmd);
                pthread_mutex_lock(&thisptr->cmd[i].mutex);
                pthread_cond_signal(&thisptr->cmd[i].cond);
                pthread_mutex_unlock(&thisptr->cmd[i].mutex);
            }
        }
        pthread_mutex_unlock(&thisptr->cmd_buf_mutex);
        if (ret == 0)
            return 0;
    }

    if (cmd->opcode == CEC_OPCODE_ACTIVE_SOURCE)
    {
        pthread_mutex_lock(&thisptr->dev_list_mutex);
        thisptr->device_list.addresses[cmd->initiator] = HUDI_CEC_DEVICE_ACTIVE;
        pthread_mutex_unlock(&thisptr->dev_list_mutex);
    }
    else if (cmd->opcode == CEC_OPCODE_INACTIVE_SOURCE)
    {
        pthread_mutex_lock(&thisptr->dev_list_mutex);
        thisptr->device_list.addresses[cmd->initiator] = HUDI_CEC_DEVICE_PING;
        pthread_mutex_unlock(&thisptr->dev_list_mutex);
    }

    {
        hudi_cec_cmd_t temp = {};
        temp.initiator      = (hudi_cec_la_e)cmd->initiator;
        temp.destination    = (hudi_cec_la_e)cmd->destination;
        temp.opcode         = (hudi_cec_opcode_e)cmd->opcode;
        memcpy(&temp.parameters, &cmd->parameters, sizeof(cec_datapacket));
        if (thisptr->msgid_action != LIB_CEC_MSGID_ACTION_NONE &&
            thisptr->msg_id > 0)
            hudi_cec_api_message_send(thisptr->msg_id, &temp,
                                      sizeof(hudi_cec_cmd_t));
    }
    return 0;
}

static void *Process(void *thisobj)
{
    hudi_cec_oprocessor_t *thisptr = (hudi_cec_oprocessor_t *)thisobj;
    while (!hudi_cec_cthread_is_stopped(&thisptr->cthread))
    {
        cec_command cmd = {};
        pthread_mutex_lock(&thisptr->msg_obj.mutex);
        while (hudi_cec_api_message_receive(thisptr->msg_obj.msg_handle, &cmd, sizeof(cec_command)) != 0 &&
               !hudi_cec_cthread_is_stopped(&thisptr->cthread))
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 2;
            pthread_cond_timedwait(&thisptr->msg_obj.cond, &thisptr->msg_obj.mutex, &ts);
        };
        pthread_mutex_unlock(&thisptr->msg_obj.mutex);
        // if get cmd
        if (!hudi_cec_cthread_is_stopped(&thisptr->cthread))
        {
            hudi_cec(HUDI_LL_DEBUG, "processor - cmd:(%d,%d,0x%x", cmd.initiator, cmd.destination,
                     cmd.opcode);
            for (uint8_t i = 0; i < cmd.parameters.size; i++)
            {
                hudi_cec_pr(HUDI_LL_DEBUG, ",0x%x", cmd.parameters.data[i]);
            }
            hudi_cec_pr(HUDI_LL_DEBUG, ")\n");

            hudi_cec_handle_cmd(thisptr, &cmd);
        }
    }

    hudi_cec(HUDI_LL_NOTICE, "processor - stopped\n");

    return NULL;
}

static void processor_close(hudi_cec_oprocessor_t *thisptr)
{
    hudi_cec_cthread_stop_thread(&thisptr->cthread, 0);
    if (thisptr->mcomm->mcommunication.IsOpen(thisptr->mcomm))
    {
        thisptr->mcomm->mcommunication.Close(thisptr->mcomm);
    }

    hudi_cec_destroy_instance(&thisptr->mcomm);
}

bool hudi_cec_processor_destory(hudi_cec_oprocessor_t *thisptr)
{
    // msg destroy
    if (thisptr->msg_obj.msg_handle > 0)
        hudi_cec_api_message_delete(&thisptr->msg_obj.msg_handle);

    pthread_mutex_destroy(&thisptr->msg_obj.mutex);
    pthread_cond_destroy(&thisptr->msg_obj.cond);
    pthread_mutex_destroy(&thisptr->dev_list_mutex);
    pthread_mutex_destroy(&thisptr->adapter_mutex);
    pthread_mutex_destroy(&thisptr->cmd_buf_mutex);

    if (thisptr->msgid_action == LIB_CEC_MSGID_ACTION_CREATE)
    {
        if (thisptr->msg_id > 0)
            hudi_cec_api_message_delete(&thisptr->msg_id);
    }

    return true;
}

bool hudi_cec_processor_init(hudi_cec_oprocessor_t *thisptr, const libcec_configuration *configuration)
{
    memset(thisptr, 0, sizeof(hudi_cec_oprocessor_t));
    bool ret = true;
    thisptr->processor.Start = processsor_start;
    thisptr->processor.Transmit = processor_transmit;
    thisptr->processor.SetCECInitialised = processor_set_cec_initialised;
    thisptr->processor.Close = processor_close;
    thisptr->processor.ResetMembers = processor_reset_members;
    thisptr->processor.OpenConnection = processor_open_connection;
    thisptr->processor.PowerOn = processor_poweron;
    thisptr->processor.KeyRelease = processor_keyrelease;
    thisptr->processor.KeyPress = processor_keypress;
    thisptr->processor.SetLogicalAddress = processor_set_logical_address;
    thisptr->processor.GetLogicalAddress = processor_get_logical_address;
    thisptr->processor.StandbyDevice = processor_standby;
    thisptr->processor.OneTouchPlay = processor_one_touch_play;
    thisptr->processor.ScanDevices = processor_scan_devices;
    thisptr->processor.GetActiveDevices = processor_get_active_devices;
    thisptr->processor.GetMsgId = processor_get_msgid;
    thisptr->processor.MsgRecv = processor_msg_recv;
    thisptr->processor.KeyPressThrough = processor_keypress_through;
    thisptr->processor.AudioVolUp = processor_audio_volup;
    thisptr->processor.AudioVolDown = processor_audio_voldown;
    thisptr->processor.AudioVolMute = processor_audio_toggle_mute;
    thisptr->processor.MarkBusy = processor_mark_busy;
    thisptr->processor.MarkReady = processor_mark_ready;
    thisptr->processor.GetDevicePowerStatus = processor_get_device_power_status;
    thisptr->processor.IsActiveSource = processor_is_active_source;
    thisptr->processor.GetDeviceVendorId = processor_get_device_vendor_id;
    thisptr->processor.SendSpecialCmd = processor_send_special_cmd;
    thisptr->processor.GetPhysicalAddress = processor_get_physical_address;
    thisptr->processor.SetProcessorStatus = processor_set_proc_status;
    thisptr->processor.CheckProcessorStatus = processor_check_proc_status;


    memcpy(&thisptr->device_list, &configuration->logical_addresses, sizeof(cec_logical_addresses));
    thisptr->msg_id = configuration->msg_id;
    thisptr->msgid_action = configuration->msgid_action;

    // processor adpter msg init
    thisptr->msg_obj.msg_handle = hudi_cec_api_message_create(CTL_MSG_COUNT, sizeof(cec_command));
    if (thisptr->msg_obj.msg_handle == 0)
    {
        hudi_cec(HUDI_LL_NOTICE, "create processor adapter msg chanel failed!\n");
        return false;
    }
    // mutex cond init
    pthread_mutex_init(&thisptr->msg_obj.mutex, NULL);
    pthread_cond_init(&thisptr->msg_obj.cond, NULL);
    pthread_mutex_init(&thisptr->dev_list_mutex, NULL);
    pthread_mutex_init(&thisptr->adapter_mutex, NULL);
    pthread_mutex_init(&thisptr->cmd_buf_mutex, NULL);

    // thread init
    hudi_cec_cthread_thread_init(&thisptr->cthread, (void *)thisptr, Process);

    // client msg init
    if (thisptr->msgid_action == LIB_CEC_MSGID_ACTION_CREATE)
    {
        thisptr->msg_id = hudi_cec_api_message_create(CTL_MSG_COUNT, sizeof(hudi_cec_cmd_t));
        if (thisptr->msg_id == 0)
        {
            hudi_cec(HUDI_LL_NOTICE, "create client msg chanel failed!\n");
            return false;
        }
    }

    return ret;
}
