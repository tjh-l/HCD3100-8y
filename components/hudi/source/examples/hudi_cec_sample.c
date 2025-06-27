#include <stdio.h>
#include "hudi/hudi_cec.h"

int hdmi_cec_test_start(void)
{
    hudi_handle                  handle = NULL;
    hudi_cec_config_t            config = {};
    hudi_cec_logical_addresses_t laes = {};
    config.dev_path     = "/dev/hdmi_rx";
    config.msgid_action = HUDI_CEC_MSGID_ACTION_CREATE;
    hudi_cec_cmd_t cmd  = {};
    int ret = 0;

    //1.open conn
    ret = hudi_cec_open(&handle, &config);
    printf("api test1:%d\n", ret);

    //2.use conn(test2 ~ test 6)
    ret = hudi_cec_standby_device(handle, HUDI_CEC_DEVICE_BROADCAST);
    printf("api test2:%d\n", ret);

    ret = hudi_cec_scan_devices(handle, &laes);
    printf("api test3:%d\n", ret);
    for (int i = 0; i < 15; i++)
    {
        if (laes.addresses[i] > 0)
        {
            printf("scan device:%d\n", i);
        }
    }

    ret = hudi_cec_poweron_device(handle, HUDI_CEC_DEVICE_BROADCAST);
    printf("api test4:%d\n", ret);

    ret = hudi_cec_get_active_devices(handle, &laes, 500);
    printf("api test5:%d\n", ret);
    for (int i = 0; i < 15; i++)
    {
        if (laes.addresses[i] > 1)
        {
            printf("active device:%d\n", i);
        }
    }

    ret = hudi_cec_msg_receive(handle, &cmd, true);
    printf("api test6:%d\n", ret);
    printf("get cec_cmd(i,d,o,p):(%d,%d,%02x,%02x)\n", cmd.initiator,
           cmd.destination, cmd.opcode, cmd.parameters.data[0]);

    //close conn
    ret = hudi_cec_close(handle);
    printf("api test7:%d\n", ret);

    return 0;
}
