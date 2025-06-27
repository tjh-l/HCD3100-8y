#include "hc_test_usb_mmc.h"
#include <stdio.h>

static volatile int msg_mmc_state;
static volatile bool user_exit;
static int hc_test_mmc_test_exit(void)
{
    user_exit = true;
    if (msg_mmc_state == SD_STAT_MOUNT) {
        return BOARDTEST_RESULT_PASS;
    }
    return BOARDTEST_PASS;
}

//Obtain SB mount information
static int fs_mmc_mount_notify(struct notifier_block *self, unsigned long action,
                               void *dev)
{
    switch (action)             
    {
        case USB_MSC_NOTIFY_MOUNT:
            if (strstr(dev, "mmc")){
                msg_mmc_state = SD_STAT_MOUNT;
            }
            break;
        case USB_MSC_NOTIFY_MOUNT_FAIL:
            if (strstr(dev, "mmc")){
                msg_mmc_state = SD_STAT_MOUNT_FAIL;
            }
            break;
        default:
            return NOTIFY_OK;
            break;
    }
    return NOTIFY_OK;
}
static struct notifier_block fs_mmc_mount =
{
    .notifier_call = fs_mmc_mount_notify,
};

/**
 * @brief Get the mmc state object
 * @description: Obtain mmc mount status and read/write speed
 * @return int 
 */
static int get_mmc_state(void)
{
    char path[512]={0};
    int mmc_state = -1;
    wr_speed mmc_speed;
    char mmc_state_describe[60];
    int mmc_cnt = 0;
    DIR * dir;
    struct dirent * ptr;

    dir = opendir("/media/");
    if (dir != NULL) {
        while((ptr = readdir(dir)) != NULL)
        {
            if (strstr(ptr->d_name, "mmc")){
                sprintf(&path[0], "/media/%s/hichip-test.bin", ptr->d_name);
                get_write_read_speed(path, 512*1024, 1024*1024*8, &mmc_speed);
                mmc_state = 0;
                mmc_cnt++;
            }
        }
        closedir(dir);
    }

    if (mmc_state == 0) {
        sprintf(&mmc_state_describe[0],
                "%d SD_mount,speed:write %d KB/s,read=%d KB/s",
                mmc_cnt, mmc_speed.write_speed, mmc_speed.read_speed);
        write_boardtest_detail(BOARDTEST_SD_TEST, mmc_state_describe);
        return BOARDTEST_PASS;
    }
    /*----------------------------------------------------------------------------------*/

    user_exit = false;
    mmc_cnt = 0;
    msg_mmc_state = USB_STAT_INVALID;
    sys_register_notify(&fs_mmc_mount);

    time_t starttime1 = time(NULL);
    time_t currenttime1;

    while (!user_exit)
    {
        currenttime1 = time(NULL);
        if(currenttime1 - starttime1 >= TEST_CLOCKS_RER_SEC){
            printf("read MMC timeout\n");
            break;
        }
        if (msg_mmc_state == SD_STAT_MOUNT) {

            dir = opendir("/media/");
            if (dir != NULL) {
                while((ptr = readdir(dir)) != NULL)
                {
                    if (strstr(ptr->d_name, "mmc")){
                        sprintf(&path[0], "/media/%s/hichip-test.bin", ptr->d_name);
                        get_write_read_speed(path, 512*1024, 1024*1024*8, &mmc_speed);
                        mmc_state = 0;
                        mmc_cnt++;
                    }
                }
                closedir(dir);
                break;
            }
        }
        usleep(100000);
    }
    
    switch (msg_mmc_state) {
    case USB_STAT_INVALID: 
        strcpy(&mmc_state_describe[0],"SD_invalid");
        break;
    case SD_STAT_MOUNT: 
        sprintf(&mmc_state_describe[0],
                "%d SD_mount,speed:write %d KB/s,read=%d KB/s",
                mmc_cnt, mmc_speed.write_speed, mmc_speed.read_speed);
        break;
    case SD_STAT_MOUNT_FAIL: 
        strcpy(&mmc_state_describe[0],"SD_mount_fail");
        break;
    default: 
        strcpy(&mmc_state_describe[0],"SD_identify_fail");
        break;
    }

    write_boardtest_detail(BOARDTEST_SD_TEST, mmc_state_describe);
    if (mmc_state == 0) {
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
static int hc_boardtest_mmc_test(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    /*If it is not needed, please assign a value to NULL*/
    test->english_name = "SD_TEST"; /*It needs to be consistent with the .ini profile*/
    test->sort_name = BOARDTEST_SD_TEST;  /*Please go to the header file to find the corresponding name*/
    test->init = NULL;
    test->run = get_mmc_state;
    test->exit = hc_test_mmc_test_exit;
    test->tips = NULL;
    // test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/

    hc_boardtest_module_register(test);

    return 0;
}

/*Automatic enrollment*/
__initcall(hc_boardtest_mmc_test);
/*----------------------------------------------------------------------------------*/