/**
 * @file
 * @brief                hudi cec module command process interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_PROCESSOR_H__
#define __HUDI_CEC_PROCESSOR_H__

#include <stdint.h>
#include "adapter/hudi_cec_adapter_communication.h"
#include "hudi_cec_types.h"
#include "utils/hudi_cec_threadutils.h"
typedef struct hudi_cec_iprocessor_t hudi_cec_iprocessor_t;
typedef struct hudi_cec_oprocessor_t hudi_cec_oprocessor_t;

typedef enum{
    HUDI_CEC_PROCESSOR_STATUS_FREE = 0,
    HUDI_CEC_PROCESSOR_STATUS_CLOSING = 1,
} hudi_cec_processor_status_e;

struct hudi_cec_iprocessor_t
{
    bool (*Transmit)(hudi_cec_oprocessor_t* thisptr, const cec_command* data, bool bneed_reply);

    bool (*Start)(hudi_cec_oprocessor_t* thisptr, const char* str_port, uint16_t ibaud_rate,
                  uint32_t itimeoutms);

    bool (*OpenConnection)(hudi_cec_oprocessor_t* thisptr, const char* str_port, uint16_t ibaud_rate,
                           uint32_t itimeoutms, bool bstart_listening /* = true */);

    void (*Close)(hudi_cec_oprocessor_t* thisptr);

    void (*ResetMembers)(hudi_cec_oprocessor_t* thisptr);

    void (*SetCECInitialised)(hudi_cec_oprocessor_t* thisptr, bool bset_to /* = true */);

    bool (*PowerOn)(hudi_cec_oprocessor_t* thisptr, const cec_logical_address destination);

    bool (*StandbyDevice)(hudi_cec_oprocessor_t* thisptr, const cec_logical_address destination);

    bool (*OneTouchPlay)(hudi_cec_oprocessor_t* thisptr);

    bool (*KeyRelease)(hudi_cec_oprocessor_t*    thisptr,
                       const cec_logical_address initiator,
                       const cec_logical_address destination, bool bWait);

    bool (*KeyPress)(hudi_cec_oprocessor_t* thisptr, const cec_logical_address initiator,
                     const cec_logical_address destination, cec_user_control_code key, bool bWait);

    bool (*KeyPressThrough)(hudi_cec_oprocessor_t*    thisptr,
                            const cec_logical_address destination,
                            cec_user_control_code     key);

    bool (*AudioVolUp)(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination);

    bool (*AudioVolDown)(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination);

    bool (*AudioVolMute)(hudi_cec_oprocessor_t *thisptr, const cec_logical_address destination);

    bool (*SetLogicalAddress)(hudi_cec_oprocessor_t* thisptr, const cec_logical_address address);

    int (*ScanDevices)(hudi_cec_oprocessor_t* thisptr, cec_logical_addresses* address);

    int (*GetActiveDevices)(hudi_cec_oprocessor_t* thisptr, cec_logical_addresses* address,
                            const uint32_t timewaitms);

    cec_logical_address (*GetLogicalAddress)(hudi_cec_oprocessor_t* thisptr);

    bool (*GetMsgId)(hudi_cec_oprocessor_t* thisptr, int* msg_id);

    bool (*MsgRecv)(hudi_cec_oprocessor_t* thisptr, void* cmd, const bool nowait);

    void (*MarkBusy)(hudi_cec_oprocessor_t* thisptr);

    void (*MarkReady)(hudi_cec_oprocessor_t* thisptr);

    bool (*GetDevicePowerStatus)(hudi_cec_oprocessor_t*    thisptr,
                                 const cec_logical_address destination,
                                 cec_power_status*         status,
                                 const int32_t             timeoutms);

    bool (*IsActiveSource)(hudi_cec_oprocessor_t*    thisptr,
                           const cec_logical_address destination);

    bool (*GetDeviceVendorId)(hudi_cec_oprocessor_t*    thisptr,
                              const cec_logical_address destination,
                              uint32_t* vendorid, const int32_t timeoutms);

    bool (*SendSpecialCmd)(hudi_cec_oprocessor_t*    thisptr,
                           const cec_logical_address destination,
                           const uint8_t opcode, const void* params);

    bool (*GetPhysicalAddress)(hudi_cec_oprocessor_t* thisptr,
                               unsigned int*          physical_address);

    int (*SetProcessorStatus)(hudi_cec_oprocessor_t* thisptr, hudi_cec_processor_status_e status);

    bool (*CheckProcessorStatus)(hudi_cec_oprocessor_t* thisptr, hudi_cec_processor_status_e status);
};

typedef struct
{
    cec_power_status status;
    unsigned int vendorid;
} hudi_cec_device_info_t;

typedef enum
{
    HUDI_CEC_COMMAND_BUFF_STATUS_FREE = 0,
    HUDI_CEC_COMMAND_BUFF_STATUS_USING = 1,
    HUDI_CEC_COMMAND_BUFF_STATUS_WAITING_RES = 2,
} hudi_cec_command_buff_status_e;
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    hudi_cec_command_buff_status_e status;

    cec_command cmd;
} hudi_cec_command_buff_t;
#define HUDI_CEC_COMMAND_BUFF_COUNT 6

struct hudi_cec_oprocessor_t
{
    hudi_cec_cthread_t                         cthread;
    hudi_cec_iadapter_communication_callback_t comm_callback;
    hudi_cec_iprocessor_t                      processor;

    bool binitialised;

    hudi_cec_oadapter_communication_t* mcomm;

    uint8_t  m_iStandardLineTimeout;
    uint8_t  m_iRetryLineTimeout;
    uint64_t m_iLastTransmission;

    bool m_bMonitor;

    bool m_bStallCommunication;

    uint32_t               msg_id;
    lib_cec_msgid_action_e msgid_action;

    hudi_cec_cthread_msg_t msg_obj;

    hudi_cec_device_info_t devices[16];
    cec_logical_addresses device_list;
    pthread_mutex_t       dev_list_mutex;
    pthread_mutex_t       adapter_mutex;

    pthread_mutex_t       cmd_buf_mutex;
    hudi_cec_command_buff_t cmd[HUDI_CEC_COMMAND_BUFF_COUNT];

    hudi_cec_processor_status_e proc_status;

};
bool hudi_cec_processor_init(hudi_cec_oprocessor_t*      thisptr,
                             const libcec_configuration* configuration);
bool hudi_cec_processor_destory(hudi_cec_oprocessor_t* thisptr);

#define HUDI_CEC_DEVICE_ACTIVE 2
#define HUDI_CEC_DEVICE_PING 1

#endif
