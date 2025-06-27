#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <generated/br2_autoconf.h>
#include <linux/delay.h>
#include <kernel/lib/fdt_api.h>
#include <boardtest_module.h>

#ifdef BR2_PACKAGE_BLUETOOTH
#include <bluetooth.h>

#define INFOMAXLEN 2048
#define BLUETOOTHTIMEOUT 30000 //30s

char *bluetoothinfo = NULL;
static volatile bool user_exit;
static volatile bool has_getdev;
static const char* detailstr;

static void bluetooth_inquiry_printf(struct bluetooth_slave_dev *data)
{
    sprintf((bluetoothinfo + strlen(bluetoothinfo)), "%s:%02x %02x %02x %02x %02x %02x | ",
            data->name, data->mac[0], data->mac[1], data->mac[2], data->mac[3], data->mac[4], data->mac[5]);
    printf("%s\n", bluetoothinfo);
    has_getdev = true;
    write_boardtest_detail(BOARDTEST_BLUETOOTH_TEST, bluetoothinfo);
}

static int bluetooth_callback_test(unsigned long event, unsigned long param)
{
    struct bluetooth_slave_dev dev_info = { 0 };
    struct bluetooth_slave_dev *dev_info_t = NULL;
    switch (event) {
        case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCANNED\n");
            if (param == 0)
                break;
            dev_info_t = (struct bluetooth_slave_dev*)param;
            memcpy(&dev_info, dev_info_t, sizeof(struct bluetooth_slave_dev));
            bluetooth_inquiry_printf(&dev_info);
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
            user_exit = true;
            break;
        default:
            break;
    }
    return 0;
}

int hc_test_bluetooth_init()
{
    detailstr = NULL;
    const char *devpath = NULL;
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if (np > 0) {
        if (!fdt_get_property_string_index(np, "devpath", 0, &devpath)) {
            if (bluetooth_init(devpath, bluetooth_callback_test) == 0) {
                printf("%s %d bluetooth_init ok\n", __FUNCTION__, __LINE__);
            } else {
                detailstr = "bluetooth init error.";
                printf("%s %d %s\n", __FUNCTION__, __LINE__, detailstr);
                return BOARDTEST_ERROR_OPEN_DEVICE;
            }
        }
    } else {
        detailstr = "bluetooth init error.";
        printf("%s %d %s\n", __FUNCTION__, __LINE__, detailstr);
        return BOARDTEST_ERROR_OPEN_DEVICE;
    }
    return BOARDTEST_PASS;
}

static int hc_test_bluetooth_scan()
{
    int ret = BOARDTEST_PASS;
    user_exit = false;
    has_getdev = false;
    int timeout = 0;

    bluetoothinfo = malloc(INFOMAXLEN);
    if (!bluetoothinfo) {
        detailstr = "Malloc memory failed.";
        ret = BOARDTEST_ERROR_MOLLOC_MEMORY;
        goto close;
    }
    bluetoothinfo[0] = '|';
    bluetoothinfo[1] = ' ';
    memset(bluetoothinfo + 2, '\0', INFOMAXLEN - 2);

    if (bluetooth_poweron() == 0) {
        printf("Device exists\n");
    } else {
        detailstr = "Bluetooth dev does not exist.";
        ret = BOARDTEST_ERROR_OPEN_DEVICE;
        goto close;
    }

    if (bluetooth_scan()) {
        detailstr = "Bluetooth scan failed.";
        ret = BOARDTEST_FAIL;
        goto close;
    }

    while(!user_exit) {
        msleep(200);
        timeout += 200;
        if(timeout >= BLUETOOTHTIMEOUT) {
            detailstr = "Bluetooth scan timeout.";
            break;
        }
    }

    if (!has_getdev) {
        ret = BOARDTEST_FAIL;
        goto close;
    }

close:
    if (bluetoothinfo)
        free(bluetoothinfo);

    return ret;
}

static int hc_test_bluetooth_exit()
{
    bluetooth_stop_scan();
    bluetooth_poweroff();
    user_exit = true;
    printf("Bluetooth exit!\n");
    write_boardtest_detail(BOARDTEST_BLUETOOTH_TEST, detailstr);
    if (has_getdev)
        return BOARDTEST_RESULT_PASS;
    else {
        if (!detailstr) {
            detailstr = "No Bluetooth device was detected, but Bluetooth is working.";
            write_boardtest_detail(BOARDTEST_BLUETOOTH_TEST, detailstr);
        }
        return BOARDTEST_FAIL;
    }
}

static int hc_boardtest_bluetooth_auto_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "BLUETOOTH_TEST";
    test->sort_name = BOARDTEST_BLUETOOTH_TEST;
    test->init = hc_test_bluetooth_init;
    test->run = hc_test_bluetooth_scan;
    test->exit = hc_test_bluetooth_exit;
    test->tips = NULL;

    hc_boardtest_module_register(test);

    return BOARDTEST_PASS;
}

__initcall(hc_boardtest_bluetooth_auto_register)

#else

static int hc_test_bluetooth_exit()
{
    write_boardtest_detail(BOARDTEST_BLUETOOTH_TEST, "The Bluetooth driver is not enabled.");
    return BOARDTEST_FAIL;
}

static int hc_boardtest_bluetooth_auto_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "BLUETOOTH_TEST";
    test->sort_name = BOARDTEST_BLUETOOTH_TEST;
    test->init = NULL;
    test->run = NULL;
    test->exit = hc_test_bluetooth_exit;
    test->tips = NULL;

    hc_boardtest_module_register(test);

    return BOARDTEST_PASS;
}

__initcall(hc_boardtest_bluetooth_auto_register)

#endif
