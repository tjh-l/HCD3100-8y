#include "hc_test_usb_mmc.h"
#include "sys/stat.h"
#include <stdio.h>

static volatile int msg_usb_state;
static volatile bool user_exit;
static int hc_test_usb_test_exit(void)
{
    user_exit = true;
    if (msg_usb_state == USB_STAT_MOUNT) {
        return BOARDTEST_RESULT_PASS;
    }
    return BOARDTEST_PASS;
}

//Obtain USB mount information
static int fs_usb_mount_notify(struct notifier_block *self, unsigned long action,
                               void *dev)
{
    switch (action)
    {
        case USB_MSC_NOTIFY_MOUNT:
            if (strstr(dev, "sd")){
                msg_usb_state = USB_STAT_MOUNT;
            }
            break;
        case USB_MSC_NOTIFY_MOUNT_FAIL:
            if (strstr(dev, "sd")){
                msg_usb_state = USB_STAT_MOUNT_FAIL;
            }
            break;
        default:
            return NOTIFY_OK;
            break;
    }
    return NOTIFY_OK;
}
static struct notifier_block fs_usb_mount =
{
    .notifier_call = fs_usb_mount_notify,
};

/**
 * @brief Get the usb state object
 * @description: Obtain USB mount status and read/write speed
 * @return int 
 */
static int get_usb_state(void)
{
    char path[512] = {0};
    int usb_state = -1;
    wr_speed usb_speed;
    char usb_state_describe[60];
    int usb_cnt = 0;
    DIR * dir;
    struct dirent * ptr;

    dir = opendir("/media/");
    if (dir != NULL) {
        while((ptr = readdir(dir)) != NULL)
        {
            if (strstr(ptr->d_name, "sd")){
                sprintf(&path[0], "/media/%s/hichip-test.bin", ptr->d_name);
                get_write_read_speed(path, 512*1024, 1024*1024*8, &usb_speed);
                usb_state = 0;
                usb_cnt++;
            }
        }
        closedir(dir);
    }

    if (usb_state == 0)
    {
        sprintf(&usb_state_describe[0],
                "%d USB_mount,speed:write %d KB/s,read=%d KB/s",
                usb_cnt, usb_speed.write_speed, usb_speed.read_speed);
        write_boardtest_detail(BOARDTEST_USB_TEST, usb_state_describe);
        return BOARDTEST_PASS;
    }
    /*----------------------------------------------------------------------------------*/

    user_exit = false;
    usb_cnt = 0;
    msg_usb_state = USB_STAT_INVALID;
    sys_register_notify(&fs_usb_mount);

    time_t starttime1 = time(NULL);
    time_t currenttime1;

    while (!user_exit)
    {
        currenttime1 = time(NULL);
        if(currenttime1 - starttime1 >= TEST_CLOCKS_RER_SEC){
            printf("read USB timeout\n");
            break;
        }
        if (msg_usb_state == USB_STAT_MOUNT) {
            dir = opendir("/media/");
            if (dir != NULL) {
                while((ptr = readdir(dir)) != NULL)
                {
                    if (strstr(ptr->d_name, "sd")){
                        sprintf(&path[0], "/media/%s/hichip-test.bin", ptr->d_name);
                        get_write_read_speed(path, 512*1024, 1024*1024*8, &usb_speed);
                        usb_state = 0;
                        usb_cnt++;
                    }
                }
                closedir(dir);
                break;
            }
        }
        usleep(100000);
    }

    switch (msg_usb_state)
    {
    case USB_STAT_INVALID:
        strcpy(&usb_state_describe[0], "USB_invalid");
        break;
    case USB_STAT_MOUNT:
        sprintf(&usb_state_describe[0],
                "%d USB_mount,speed:write %d KB/s,read=%d KB/s",
                usb_cnt, usb_speed.write_speed, usb_speed.read_speed);
        break;
    case USB_STAT_MOUNT_FAIL:
        strcpy(&usb_state_describe[0], "USB_mount_fail");
        break;
    default:
        strcpy(&usb_state_describe[0], "USB_identify_fail");
        break;
    }

    write_boardtest_detail(BOARDTEST_USB_TEST, usb_state_describe);
    if (usb_state == 0) {
        return BOARDTEST_PASS;
    }else {
        return BOARDTEST_FAIL;
    }
    return 0;
}

/*Invoke the template*/
/*----------------------------------------------------------------------------------*/
/**
 * @brief Function naming rules : hc_boardtest_<module>_auto_register
 */
static int hc_boardtest_usb_test(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    /*If it is not needed, please assign a value to NULL*/
    test->english_name = "USB_TEST"; /*It needs to be consistent with the .ini profile*/
    test->sort_name = BOARDTEST_USB_TEST;  /*Please go to the header file to find the corresponding name*/
    test->init = NULL;
    test->run = get_usb_state;
    test->exit = hc_test_usb_test_exit;
    test->tips = NULL;
    // test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/

    hc_boardtest_module_register(test);

    return 0;
}

/*Automatic enrollment*/
__initcall(hc_boardtest_usb_test);
/*----------------------------------------------------------------------------------*/