#ifndef _HC_TEST_CVBS_IN_H_
#define _HC_TEST_CVBS_IN_H_

/*****************  Function   **********************/
/**
 * @brief cvbs_in测试初始化
 *
 * @param 无参
 * @return   BOARDTEST_FAIL：cvbs_in初始化失败
 *           BOARDTEST_PASS:cvbs_in初始化成功
 */
int hc_test_cvbs_in_init(void);

/**
 * @brief cvbs_in测试开始
 *
 * @param 无参
 * @return   BOARDTEST_FAIL：获取cvbs_in测试失败
 *           BOARDTEST_PASS:获取cvbs_in测试成功
 */
int hc_test_cvbs_in_start(void);

/**
 * @brief cvbs_in接入状态测试退出
 *
 * @param 无参
 * @return   BOARDTEST_PASS：cvbs_in接入状态测试退出函数运行成功
 *           BOARDTEST_FAIL: cvbs_in接入状态测试退出函数运行失败
 */
int hc_test_cvbs_in_stop(void);

#endif
