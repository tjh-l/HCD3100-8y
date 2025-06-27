#include "bl_test.h"

static int btl_test_func_init(void)
{
	int j = 0;

	struct test_funcs *func = btl_test_func_list[0];
	int func_count =
		sizeof(btl_test_func_list) / sizeof(btl_test_func_list[0]);

	BTL_TEST_INFO("init test function");
	if (0 == func_count) {
		BTL_TEST_SAVE_ERR("test functions list is NULL, fail\n");
		return -ENODATA;
	}

	btl_ftest = (struct btl_test *)kzalloc(sizeof(*btl_ftest), GFP_KERNEL);
	if (NULL == btl_ftest) {
		BTL_TEST_ERROR("malloc memory for test fail");
		return -ENOMEM;
	}

	for (i = 0; i < func_count; i++) {
		func = btl_test_func_list[i];
		for (j = 0; j < BTL_MAX_COMPATIBLE_TYPE; j++) {
			if (0 == func->ctype[j])
				break;
			else if (func->ctype[j] == g_btl_ts->chipInfo.chipID) {
				BTL_TEST_INFO("match test function,type:%x",
					      (int)func->ctype[j]);
				btl_ftest->func = func;
			}
		}
	}
	if (NULL == btl_ftest->func) {
		BTL_TEST_ERROR("no test function match, can't test");
		return -ENODATA;
	}

	return 0;
}

int btl_test_init(void)
{
	int ret = 0;

	BTL_TEST_FUNC_ENTER();
	/* get test function, must be the first step */
	ret = btl_test_func_init();
	if (ret < 0) {
		BTL_TEST_SAVE_ERR("test functions init fail");
		return ret;
	}
	BTL_TEST_FUNC_EXIT();

	return ret;
}
