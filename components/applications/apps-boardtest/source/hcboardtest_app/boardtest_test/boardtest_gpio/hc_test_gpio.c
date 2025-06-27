#include "boardtest_audio/hc_test_getfile.h"

static bool user_exit;
pinpad_e ionum = PINPAD_MAX;
static const char* detailstr;

int hc_test_gpio_init()
{
    char* ini_path = NULL;
    dictionary *ini = NULL;
    char section[100] = {0};
    const char *key = ":LedPin";
    int ret = BOARDTEST_PASS;
    detailstr = "";

    hc_test_findfile(&ini_path, BOARDTEST_INI_NAME);
    if(!ini_path) {
        detailstr = "can't find ini file";
        ret = BOARDTEST_ERROR_MOLLOC_MEMORY;
        goto close;
    }
    printf("ini path:%s\n", ini_path);

    ini = iniparser_load(ini_path);
    if (!ini) {
        detailstr = "can't parse ini file";
        ret = BOARDTEST_FAIL;
        goto close;
    }

    sprintf(section,"LED_DISPLAY:LedPin");
    ionum = iniparser_getint(ini, section, PINPAD_MAX);
    if (ionum == PINPAD_MAX) {
        detailstr = "can't find ionum,please specify in the ini file";
        ret = BOARDTEST_FAIL;
        goto close;
    }

close:
    if (ini)
        iniparser_freedict(ini);
    if (ini_path)
        free(ini_path);

    return ret;
}

static int hc_test_gpio_exit(void)
{
    user_exit = true;
    write_boardtest_detail(BOARDTEST_LED_DISPLAY, detailstr);
    return BOARDTEST_PASS;
}

static int hc_test_gpio_toggle(void)
{
    user_exit = false;
    if (ionum >= PINPAD_MAX) {
        return BOARDTEST_FAIL;
    }
    /* set output dir */
    gpio_configure(ionum, GPIO_DIR_OUTPUT);

    create_boardtest_passfail_mbox(BOARDTEST_LED_DISPLAY);
    /* toggle gpio */
    while(!user_exit) {
        gpio_set_output(ionum,1);
        msleep(500);
        gpio_set_output(ionum,0);
        msleep(500);
    }

    return BOARDTEST_PASS;
}

static int hc_boardtest_led_auto_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "LED_DISPLAY";
    test->sort_name = BOARDTEST_LED_DISPLAY;
    test->init = hc_test_gpio_init;
    test->run = hc_test_gpio_toggle;
    test->exit = hc_test_gpio_exit;
    test->tips = "Whether the LED toggled?";

    hc_boardtest_module_register(test);

    return 0;
}

__initcall(hc_boardtest_led_auto_register)
