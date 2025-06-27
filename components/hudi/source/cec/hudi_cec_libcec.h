/**
 * @file
 * @brief                hudi cec module inter entry interface.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_LIBCEC_H__
#define __HUDI_CEC_LIBCEC_H__

#include <stdint.h>
#include "hudi_cec_processor.h"
#include "hudi_cec_types.h"

typedef struct hudi_cec_libcec_t   hudi_cec_libcec_t;
typedef struct hudi_cec_iadapter_t hudi_cec_iadapter_t;
struct hudi_cec_iadapter_t
{
    bool (*IsActiveDevice)(cec_logical_address ilogical_address);

    bool (*Open)(hudi_cec_libcec_t *thisptr, const char *str_port, uint32_t itimeoutms);

    void (*Close)(hudi_cec_libcec_t *thisptr);

    bool (*PingAdapter)(void);

    bool (*Transmit)(hudi_cec_libcec_t *thisptr, const cec_command *data);

    bool (*SetLogicalAddress)(hudi_cec_libcec_t *thisptr, cec_logical_address ilogical_address);

    bool (*SetPhysicalAddress)(uint16_t iPhysicalAddress);

    bool (*PowerOnDevice)(hudi_cec_libcec_t *thisptr, const cec_logical_address address);

    bool (*AudioVolUp)(hudi_cec_libcec_t *thisptr, const cec_logical_address address);

    bool (*AudioVolDown)(hudi_cec_libcec_t *thisptr, const cec_logical_address address);

    bool (*AudioToggleMute)(hudi_cec_libcec_t *thisptr, const cec_logical_address address);

    bool (*StandbyDevice)(hudi_cec_libcec_t *thisptr, const cec_logical_address address);

    bool (*OneTouchPlay)(hudi_cec_libcec_t *thisptr);

    int (*ScanDevices)(hudi_cec_libcec_t *thisptr, cec_logical_addresses *laes);

    int (*GetActiveDevices)(hudi_cec_libcec_t *thisptr, cec_logical_addresses *laes,
                             const uint32_t timewaitms);

    bool (*GetMsgId)(hudi_cec_libcec_t *thisptr, int *msg_id);

    bool (*MsgRecv)(hudi_cec_libcec_t *thisptr, void *cmd, bool nowait);

    bool (*KeyPressThrough)(hudi_cec_libcec_t *         thisptr,
                            const cec_logical_address   address,
                            const cec_user_control_code key);

    bool (*GetDevicePowerStatus)(hudi_cec_libcec_t *         thisptr,
                                 const cec_logical_address   address,
                                 cec_power_status *status,
                                 const int32_t timeoutms);

    bool (*IsActiveSource)(hudi_cec_libcec_t *       thisptr,
                           const cec_logical_address address);

    bool (*GetDeviceVendorId)(hudi_cec_libcec_t *       thisptr,
                              const cec_logical_address address,
                              uint32_t *vendorid, const int32_t timeoutms);

    bool (*SendSpecialCmd)(hudi_cec_libcec_t *       thisptr,
                           const cec_logical_address address,
                           const uint8_t opcode, const void *params);

    cec_logical_address (*GetLogicalAddress)(hudi_cec_libcec_t *thisptr);
};

typedef enum{
    HUDI_CEC_CONN_STATUS_FREE = 0,
    HUDI_CEC_CONN_STATUS_CLOSING = 1,
} hudi_cec_libcec_conn_status_e;

typedef struct{
    pthread_mutex_t       ref_mutex;
    hudi_cec_libcec_conn_status_e conn_status;
    int ref_count;

} hudi_cec_libcec_ref_obj_t;

struct hudi_cec_libcec_t
{
    hudi_cec_iadapter_t *  iadapter;
    hudi_cec_oprocessor_t *mcec;
    hudi_cec_libcec_ref_obj_t ref_obj;
};

int hudi_cec_destroy(void *conn);

int hudi_cec_initialise(void *conn, const libcec_configuration *configuration);

#endif