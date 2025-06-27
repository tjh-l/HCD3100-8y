#include "boardtest_read_ini.h"
#include "boardtest_module.h"
#include "boardtest_run.h"
#include "iniparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int boardtest_read_ini(void *dev) /*sda sda1*/
{
    char ini_path[100];
    dictionary *ini = NULL;
    char section[100] = {0};
    const char *key = ":IsAbled";
    int sort;
    bool isabled;
    hc_boardtest_msg_t *boardtest;

    snprintf(ini_path, sizeof(ini_path), "media/%s/%s", dev, BOARDTEST_INI_NAME);

    ini = iniparser_load(ini_path);
    if (ini == NULL)
    {
        printf("cannot parse file: %s\n", ini_path);
        return -1;
    }
    for (int sort = 0; sort < BOARDTEST_NUM; sort++)
    {
        boardtest = hc_boardtest_msg_get(sort);
        /*The selections made by the user will not be deselected*/
        if (boardtest->isabled == BOARDTEST_ENABLE)
            continue;
        if (boardtest->boardtest_msg_reg->english_name)
        {
            strncpy(section, boardtest->boardtest_msg_reg->english_name, sizeof(section) - 1);
            strncat(section, key, sizeof(section) - strlen(section));
            isabled = iniparser_getboolean(ini, section, 0); /*0 == no find*/
        }
        else
            isabled = 0;
        boardtest->isabled = isabled;
    }
    isabled = iniparser_getboolean(ini, "AUTO:IsAbled", 0); /*0 == no find*/
    if (isabled)
        boardtest_run_set_auto();
    iniparser_freedict(ini);
    return 0;
}
