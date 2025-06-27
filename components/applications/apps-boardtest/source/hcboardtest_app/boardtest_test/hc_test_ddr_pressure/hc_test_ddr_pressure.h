#ifndef _HC_TEST_DDR_PRESSURE_
#define _HC_TEST_DDR_PRESSURE_

/*****************  Function   **********************/
/**
 * @brief ddr压力测试
 *
 * @param	
 			无参（默认ddr大小是free的70%，测试时间是12h）
 * @return	
            BOARDTEST_FAIL:ddr压力测试失败
            BOARDTEST_CALL_PASS:ddr压力测试成功
 */
int hc_test_ddr_pressure_start(void);

/**
 * @brief 退出ddr压力测试
 *
 * @param	
 			无参
 * @return	
            BOARDTEST_FAIL:退出ddr压力测试函数失败
            BOARDTEST_PASS:退出ddr压力测试函数成功
 */
int hc_test_ddr_pressure_stop(void);

void hc_test_fixup_ddr_setting(void);


#endif
