#ifndef _HC_TEST_HDMI_IN_H_
#define _HC_TEST_HDMI_IN_H_

/*****************  Function   **********************/
/**
 * @brief hdmi_in接入状态初始化
 *
 * @param 无参
 * @return  BOARDTEST_FAIL：hdmi_in接入状态初始化失败
 *          BOARDTEST_PASS: hdmi_in接入状态初始化成功
 */
int hc_test_hdmi_in_init(void);

/**
 * @brief hdmi_in测试开始
 *
 * @param 无参
 * @return   BOARDTEST_PASS：获取hdmi_in测试运行成功
 *           BOARDTEST_FAIL: 获取hdmi_in测试运行失败
 */
int hc_test_hdmi_in_start(void);

/**
 * @brief hdmi_in测试退出
 *
 * @param 无参
 * @return   BOARDTEST_PASS：hdmi_in测试退出函数运行成功
 *           BOARDTEST_FAIL: hdmi_in测试退出函数运行失败
 */
int hc_test_hdmi_in_stop(void);

#endif
