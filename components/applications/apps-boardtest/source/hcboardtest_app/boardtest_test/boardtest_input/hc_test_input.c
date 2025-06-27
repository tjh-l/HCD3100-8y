#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>

#include <time.h>
#include <string.h>

#include "key_event.h"
#include "hc_test_input.h"
#include "../boardtest_module.h"

static volatile bool user_exit;
static int hc_test_input_test_exit(void)
{
    user_exit = true;
    return BOARDTEST_RESULT_PASS;
}

static int hc_test_input_test_key(void)
{
    user_exit = false;
    KEY_MSG_s *key_msg = NULL;
    int input_dev_cnt = 0;
    uint8_t input_dev_name[32];
    int input_state[10]={0};
    int input_cnt = 0;
    char input_state_describe[50];
    int input_code = 0;
    int input_type = -1;
    char input_name[10];

    int key_fds[INPUT_EVENT_DEV_MAX] ={0};
    int i;

    for (i = 0; i < INPUT_EVENT_DEV_MAX; i++)
    {
        sprintf(input_dev_name, "/dev/input/event%d", i);
        key_fds[i] = open(input_dev_name, O_RDONLY);
        if (key_fds[i] < 0)
            break;
        input_dev_cnt++;
    }
    if (0 == input_dev_cnt)
    {
        strcpy(&input_state_describe[0], "No input/event driver");
        write_boardtest_detail(BOARDTEST_INPUT_TEST, input_state_describe);
        return BOARDTEST_PASS;
    }

    key_toggle_user_input_flag();

    time_t starttime1 = time(NULL);
    time_t currenttime1;

    while (!user_exit)
    {
        //Limited time waiting
        currenttime1 = time(NULL);
        if(currenttime1 - starttime1 >= CLOCKS_RER_SEC){
            printf("read input timeout\n");
            break;
        }

        if (key_get_user_input_flag()) {
            key_msg = api_key_msg_get();
        }

        if (key_msg){
            input_state[key_msg->key_type]=true;
            if (key_msg->key_event.type==EV_KEY && key_msg->key_event.code==KEY_EXIT) {
                break;
            }
            input_type = key_msg->key_type;
            input_code = key_msg->key_event.code;
        }
        usleep(10000);
    }
    key_toggle_user_input_flag();

    for (i = 0; i < input_dev_cnt; i++)
    {
        if (key_fds[i] > 0){
            close(key_fds[i]);
        }
        if (input_state[i]) {
            input_cnt++;
            switch (i) {
            case KEY_TYPE_IR:strcat(input_name, "IR ");break;
            case KEY_TYPE_ADC:strcat(input_name, "ADC ");break;
            case KEY_TYPE_GPIO:strcat(input_name, "GPIO ");break;
            }
        }
    }

    if (input_cnt == 0) {
        sprintf(input_state_describe, "NO input");
    }else {
        sprintf(input_state_describe, "Input: %s; last-key:dev= event%d, code= %d",input_name, input_type, input_code);
    }

    write_boardtest_detail(BOARDTEST_INPUT_TEST, input_state_describe);
    if (input_cnt == 0 || input_cnt > input_dev_cnt) {
        return BOARDTEST_FAIL;
    }
    return BOARDTEST_PASS;
}

/*Invoke the template*/
/*----------------------------------------------------------------------------------*/
/**
 * @brief Function naming rules : hc_boardtest_<module>_auto_register
 */
static int hc_boardtest_input_test(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    /*If it is not needed, please assign a value to NULL*/
    test->english_name = "INPUT_TEST"; /*It needs to be consistent with the .ini profile*/
    test->sort_name = BOARDTEST_INPUT_TEST;  /*Please go to the header file to find the corresponding name*/
    test->init = NULL;
    test->run = hc_test_input_test_key;
    test->exit = hc_test_input_test_exit;
    test->tips = NULL;
    // test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/

    hc_boardtest_module_register(test);
    return 0;
}
/*Automatic enrollment*/
__initcall(hc_boardtest_input_test);
/*----------------------------------------------------------------------------------*/