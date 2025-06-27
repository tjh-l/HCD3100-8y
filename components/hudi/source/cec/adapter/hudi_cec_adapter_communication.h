/**
 * @file
 * @brief                hudi cec module adapter interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_ADAPTER_COMMUNICATION_H__
#define __HUDI_CEC_ADAPTER_COMMUNICATION_H__

#include <stdbool.h>
#include <stdint.h>
#include "../hudi_cec_types.h"
#include "../utils/hudi_cec_threadutils.h"

typedef enum
{
    ADAPTER_MESSAGE_STATE_UNKNOWN,            /**< the initial state */
    ADAPTER_MESSAGE_STATE_WAITING_TO_BE_SENT, /**< waiting in the send queue of
                                                 the adapter, or timed out */
    ADAPTER_MESSAGE_STATE_SENT,               /**< sent and waiting on an ACK */
    ADAPTER_MESSAGE_STATE_SENT_NOT_ACKED,     /**< sent, but failed to ACK */
    ADAPTER_MESSAGE_STATE_SENT_ACKED,         /**< sent, and ACK received */
    ADAPTER_MESSAGE_STATE_INCOMING, /**< received from another device */
    ADAPTER_MESSAGE_STATE_ERROR     /**< an error occurred */
} hudi_cec_adapter_message_state_e;

typedef struct hudi_cec_oadapter_communication_t hudi_cec_oadapter_communication_t;

typedef struct hudi_cec_iadapter_communication_callback_t hudi_cec_iadapter_communication_callback_t;

struct hudi_cec_iadapter_communication_callback_t
{
    bool (*OnCommandReceived)(const cec_command *command);

    void (*HandlePoll)(cec_logical_address initiator,
                       cec_logical_address destination);

    bool (*HandleReceiveFailed)(cec_logical_address initiator);

    void (*HandleLogicalAddressLost)(cec_logical_address oldAddress);

    void (*HandlePhysicalAddressChanged)(uint16_t iNewAddress);
};

typedef struct iadapter_communication_t iadapter_communication_t;
struct iadapter_communication_t
{
    bool (*Open)(hudi_cec_oadapter_communication_t *thisptr, uint32_t itimeoutms,
                 bool bskip_checks, bool bstart_listening);

    void (*Close)(hudi_cec_oadapter_communication_t *thisptr);

    bool (*IsOpen)(hudi_cec_oadapter_communication_t *thisptr);

    hudi_cec_adapter_message_state_e (*Write)(hudi_cec_oadapter_communication_t *thisptr,
                                              const cec_command *data, bool bretry,
                                              uint8_t iline_timeout, bool bis_reply);

    bool (*SetLineTimeout)(uint8_t iTimeout);

    bool (*StartBootloader)(void);

    bool (*SetLogicalAddresses)(hudi_cec_oadapter_communication_t *      thisptr,
                                const cec_logical_addresses *addresses);

    int (*GetLogicalAddresses)(hudi_cec_oadapter_communication_t *thisptr,
                               cec_logical_address *              address);

    bool (*PingAdapter)(void);

    uint16_t (*GetFirmwareVersion)(void);

    uint32_t (*GetFirmwareBuildDate)(void);

    bool (*IsRunningLatestFirmware)(void);

    bool (*SetControlledMode)(bool controlled);

    bool (*SaveConfiguration)(const libcec_configuration *configuration);

    bool (*SetAutoMode)(bool automode);

    bool (*GetConfiguration)(libcec_configuration *configuration);

    char *(*GetPortName)(void);

    int (*GetPhysicalAddress)(hudi_cec_oadapter_communication_t *thisptr,
                              unsigned int *physical_address);

    cec_vendor_id (*GetVendorId)(hudi_cec_oadapter_communication_t *thisptr);

    bool (*SupportsSourceLogicalAddress)(const cec_logical_address address);

    cec_adapter_type (*GetAdapterType)(void);

    uint16_t (*GetAdapterVendorId)(void);

    uint16_t (*GetAdapterProductId)(void);

    void (*SetActiveSource)(bool bSetTo, bool bClientUnregistered);
};

struct hudi_cec_oadapter_communication_t
{
    hudi_cec_cthread_t            cthread;
    hudi_cec_iadapter_communication_callback_t mcallback;
    iadapter_communication_t         mcommunication;
    int                           mfd;
    hudi_cec_cthread_msg_t *      msg_obj;
    char *                        dev_path;
};

int  hudi_cec_get_instance(hudi_cec_oadapter_communication_t **comm, hudi_cec_cthread_msg_t *msg_obj,
                           const char *str_port, uint16_t ibaud_rate);
void hudi_cec_destroy_instance(hudi_cec_oadapter_communication_t **comm);

#endif