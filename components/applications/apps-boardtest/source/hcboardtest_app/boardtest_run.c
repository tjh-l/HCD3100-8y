#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boardtest_module.h"
#include "boardtest_run.h"
#include "com_api.h"
#include "osd_com.h"

static struct timespec start_time, end_time;
static int sort = -1;
static bool auto_flag = 0; /*Automatic test process flags*/
static int run_result;
static hc_boardtest_msg_t *boardtest;

static void *_boardest_run_task(void *arg)
{
    control_msg_t ctl_msg = {0};
    int ret = -1;
    boardtest = hc_boardtest_msg_get(0);

    while (1)
    {
        ret = boardtest_run_control_receive_msg(&ctl_msg);
        /*wait last test over*/
        if (boardtest->state != BOARDTEST_GOING)
        {
            if (0 == ret && auto_flag == 0)
            {
                ret = -1;

                switch (ctl_msg.msg_type)
                {
                    case MSG_TYPE_BOARDTEST_SORT_SEND:
                    {
                        sort = ctl_msg.msg_code;
                        boardtest = hc_boardtest_msg_get(sort);
                        break;
                    }
                    case MSG_TYPE_BOARDTEST_AUTO:
                    {
                        sort = -1;
                        auto_flag = 1;
                        break;
                    }
                    default:
                        break;
                }
            }
            else if (0 != ret && auto_flag == 0)
                continue;

            if (auto_flag)
            {
                while (1)
                {
                    sort++;
                    if (sort == BOARDTEST_NUM) /*Maximum test item exceeded*/
                    {
                        auto_flag = 0;
                        ctl_msg.msg_type = MSG_TYPE_BOARDTEST_AUTO_OVER;
                        api_control_send_msg(&ctl_msg);
                        break;
                    }
                    boardtest = hc_boardtest_msg_get(sort);
                    if (boardtest->isabled == BOARDTEST_ENABLE)
                        break;
                }

                if (auto_flag == 0)
                    continue;
            }
        }
        else
            continue;

        boardtest->state = BOARDTEST_GOING;
        /*update lvgl test state*/
        ctl_msg.msg_type = MSG_TYPE_BOARDTEST_EXIT;
        ctl_msg.msg_code = sort;
        api_control_send_msg(&ctl_msg);

        run_result = BOARDTEST_PASS;
        /*CLOCK_MONOTONIC represents monotonically increasing time, not affected by system time adjustments*/
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        if (boardtest->boardtest_msg_reg->init)
            run_result = boardtest->boardtest_msg_reg->init();
        printf("%d init over\n", sort);

        if (run_result == BOARDTEST_CALL_PASS || run_result == BOARDTEST_PASS) /*If an error occurs, execute the exit function directly*/
        {
            if (boardtest->boardtest_msg_reg->run)
                run_result = boardtest->boardtest_msg_reg->run();
            printf("%d run over\n", sort);
        }

        ctl_msg.msg_type = MSG_TYPE_BOARDTEST_RUN_OVER;
        ctl_msg.msg_code = run_result;
        boardtest_exit_control_send_msg(&ctl_msg);
    }
}

static void *_boardest_exit_task(void *arg)
{
    int exit_result = BOARDTEST_PASS;
    int exit_func_result;
    control_msg_t ctl_msg = {0};
    int ret = -1;
    double elapsed_time;
    bool exit_wait_flag = 0;
    bool run_over_flag = 0;
    bool mbox_enbale = 0;
    bool stress_test = 0;

    while (1)
    {
        ret = boardtest_exit_control_receive_msg(&ctl_msg);

        if (0 == ret)
        {
            switch (ctl_msg.msg_type)
            {
                case MSG_TYPE_MBOX_RESULT: // There is no consideration for whether the results match
                {
                    exit_result = ctl_msg.msg_code;
                    exit_wait_flag = 1;
                    break;
                }
                case MSG_TYPE_BOARDTEST_STOP:
                {
                    if (boardtest->state == BOARDTEST_GOING)
                    {
                        printf("force stop\n");
                        exit_result = ctl_msg.msg_code;

                        if (mbox_enbale) /*if have mbox, close mbox*/
                        {
                            /*close mbox*/
                            ctl_msg.msg_type = MSG_TYPE_MBOX_CLOSE;
                            ctl_msg.msg_code = sort;
                            api_control_send_msg(&ctl_msg);
                            mbox_enbale = 0;
                        }

                        if (stress_test) /*if stress test, auto_flag does not change*/
                        {
                            stress_test = 0;
                            if (boardtest->boardtest_msg_reg->tips) /*if have detail, create mbox*/
                            {
                                create_boardtest_passfail_mbox(sort);
                                break;
                            }
                        }

                        /*Guarantees that multiple threads will not write auto_flag at the same time*/
                        // auto_flag = 0; //don't exit the automatic test, all enabled test items will be tested
                        exit_wait_flag = 1;
                    }
                    break;
                }
                case MSG_TYPE_PASSFAIL_MBOX_CREATE:
                {
                    mbox_enbale = 1;
                    break;
                }
                case MSG_TYPE_BOARDTEST_RUN_OVER:
                {
                    if (boardtest->state == BOARDTEST_GOING)
                    {
                        run_over_flag = 1;
                        exit_result = ctl_msg.msg_code;

                        if (mbox_enbale) /*if have mbox, wait mbox result*/
                        {
                            /*if run function result is error, close mbox*/
                            if (exit_result != BOARDTEST_CALL_PASS && exit_result != BOARDTEST_PASS)
                            {
                                /*close mbox*/
                                ctl_msg.msg_type = MSG_TYPE_MBOX_CLOSE;
                                ctl_msg.msg_code = sort;
                                api_control_send_msg(&ctl_msg);
                                exit_wait_flag = 1;
                                mbox_enbale = 0;
                            }
                        }
                        else if (exit_result == BOARDTEST_CALL_PASS)
                            stress_test = 1;
                        else
                            exit_wait_flag = 1;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else
            continue;

        if (exit_wait_flag)
            exit_wait_flag = 0;
        else
            continue;

        stress_test = 0;

        if (boardtest->boardtest_msg_reg->exit) /*Overwrite when an error occurs, and do not overwrite with normal exit*/
        {
            exit_func_result = boardtest->boardtest_msg_reg->exit();

            if (exit_func_result != BOARDTEST_CALL_PASS && exit_func_result != BOARDTEST_PASS)
                exit_result = exit_func_result;
            if (exit_result == BOARDTEST_RESULT_PASS)
                exit_result = BOARDTEST_PASS;
        }

        printf("%d exit over\n", sort);

        /*When exit completes and the run function does not, wait for the run function result*/
        while (!run_over_flag)
        {
            ret = boardtest_exit_control_receive_msg(&ctl_msg);

            if (0 == ret)
            {
                ret = -1;

                switch (ctl_msg.msg_type)
                {
                    case MSG_TYPE_BOARDTEST_RUN_OVER:
                    {
                        if (boardtest->state == BOARDTEST_GOING)
                            exit_wait_flag = 1;
                        break;
                    }

                    default:
                        break;
                }

                if (exit_wait_flag)
                {
                    exit_wait_flag = 0;
                    break;
                }
            }
        }

        run_over_flag = 0;

        if (run_result != BOARDTEST_CALL_PASS && run_result != BOARDTEST_PASS)
            exit_result = run_result;
        if (run_result == BOARDTEST_RESULT_PASS)
            exit_result = BOARDTEST_PASS;

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        // run time (in milliseconds)
        elapsed_time = (end_time.tv_sec * 1000.0 + end_time.tv_nsec / 1000000.0) - (start_time.tv_sec * 1000.0 + start_time.tv_nsec / 1000000.0);

        /*save the exit_result pass or fail*/
        boardtest->state = exit_result;
        boardtest->run_time = (int)elapsed_time;
        /*Send an end-of-test message to the LVGL main thread to update the interface information*/
        ctl_msg.msg_type = MSG_TYPE_BOARDTEST_EXIT;
        ctl_msg.msg_code = sort;
        api_control_send_msg(&ctl_msg);

        mbox_enbale = 0;
        exit_result = BOARDTEST_PASS;
    }
}

void boardtest_run_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // release task resource itself

    if (pthread_create(&thread_id, &attr, _boardest_run_task, NULL))
    {
        return;
    }

    pthread_attr_destroy(&attr);
}

void boardtest_exit_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // release task resource itself

    if (pthread_create(&thread_id, &attr, _boardest_exit_task, NULL))
    {
        return;
    }

    pthread_attr_destroy(&attr);
}

void boardtest_run_set_auto(void)
{
    auto_flag = 1;
}

bool boardtest_run_get_auto(void)
{
    return auto_flag;
}
