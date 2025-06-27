#include <pthread.h>
#include <hcuapi/input.h>
#include <sys/poll.h>
#include <fcntl.h>
#include "key_event.h"
#include "com_api.h"
#include "lvgl/lvgl.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

static lv_indev_t *indev_keypad;

int key_regist(lv_group_t *group)
{
    lv_indev_set_group(indev_keypad, group);
    lv_group_set_default(group);
    return 0;
}

uint32_t key_convert_vkey(uint32_t lv_key)
{
    uint32_t vkey = VKEY_NULL;
    if (lv_key & USER_KEY_FLAG)
    {
        lv_key = lv_key & 0xFFFF;
        switch (lv_key)
        {
            case KEY_NUMERIC_0:
                vkey = V_KEY_0;
                break;
            case KEY_NUMERIC_1:
                vkey = V_KEY_1;
                break;
            case KEY_NUMERIC_2:
                vkey = V_KEY_2;
                break;
            case KEY_NUMERIC_3:
                vkey = V_KEY_3;
                break;
            case KEY_NUMERIC_4:
                vkey = V_KEY_4;
                break;
            case KEY_NUMERIC_5:
                vkey = V_KEY_5;
                break;
            case KEY_NUMERIC_6:
                vkey = V_KEY_6;
                break;
            case KEY_NUMERIC_7:
                vkey = V_KEY_7;
                break;
            case KEY_NUMERIC_8:
                vkey = V_KEY_8;
                break;
            case KEY_NUMERIC_9:
                vkey = V_KEY_9;
                break;
            case KEY_POWER:
                vkey = V_KEY_POWER;
                break;
            case KEY_AUDIO:
                vkey = V_KEY_AUDIO;
                break;
            case KEY_MUTE:
                vkey = V_KEY_MUTE;
                break;
            case KEY_ZOOM:
                vkey = V_KEY_ZOOM;
                break;
            case KEY_SUBTITLE:
                vkey = V_KEY_SUB;
                break;
            case KEY_TV:
                vkey = V_KEY_TVRADIO;
                break;
            case KEY_TEXT:
                vkey = V_KEY_TEXT;
                break;
            case KEY_LIST:
                vkey = V_KEY_LIST;
                break;
            case KEY_MENU:
                vkey = V_KEY_MENU;
                break;
            case KEY_EXIT:
                vkey = V_KEY_EXIT;
                break;
            case KEY_EPG:
                vkey = V_KEY_EPG;
                break;
            case KEY_AGAIN:
                vkey = V_KEY_RECALL;
                break;
            case KEY_FAVORITES:
                vkey = V_KEY_FAV;
                break;
            case KEY_LEFTSHIFT:
                vkey = V_KEY_FB;
                break;
            case KEY_RIGHTSHIFT:
                vkey = V_KEY_FF;
                break;
            case KEY_PLAY:
                vkey = V_KEY_PLAY;
                break;
            case KEY_PAUSE:
                vkey = V_KEY_PAUSE;
                break;
            case KEY_STOP:
                vkey = V_KEY_STOP;
                break;
            case KEY_RECORD:
                vkey = V_KEY_RECORD;
                break;
            case KEY_RED:
                vkey = V_KEY_RED;
                break;
            case KEY_GREEN:
                vkey = V_KEY_GREEN;
                break;
            case KEY_YELLOW:
                vkey = V_KEY_YELLOW;
                break;
            case KEY_BLUE:
                vkey = V_KEY_BLUE;
                break;
            case KEY_HOME:
                vkey = V_KEY_HOME;
                break;
        }
    }
    else
    {
        switch (lv_key)
        {
            case LV_KEY_UP:
                vkey = V_KEY_UP;
                break;
            case LV_KEY_DOWN:
                vkey = V_KEY_DOWN;
                break;
            case LV_KEY_LEFT:
                vkey = V_KEY_LEFT;
                break;
            case LV_KEY_RIGHT:
                vkey = V_KEY_RIGHT;
                break;
            case LV_KEY_NEXT:
                vkey = V_KEY_NEXT;
                break;
            case LV_KEY_PREV:
                vkey = V_KEY_PREV;
                break;
            case LV_KEY_ENTER:
                vkey = V_KEY_ENTER;
                break;
            case LV_KEY_ESC:
                vkey = V_KEY_EXIT;
                break;
        }
    }

    return vkey;
}


//////////////////////////////use for tinyplayer case//////////////////////////

uint32_t key_convert2_vkey(uint32_t lv_key)
{
    uint32_t vkey = VKEY_NULL;
    if (lv_key & USER_KEY_FLAG)
    {
        lv_key = lv_key & 0xFFFF;
        switch (lv_key)
        {
            case KEY_NUMERIC_0:
                vkey = V_KEY_0;
                break;
            case KEY_NUMERIC_1:
                vkey = V_KEY_1;
                break;
            case KEY_NUMERIC_2:
                vkey = V_KEY_2;
                break;
            case KEY_NUMERIC_3:
                vkey = V_KEY_3;
                break;
            case KEY_NUMERIC_4:
                vkey = V_KEY_4;
                break;
            case KEY_NUMERIC_5:
                vkey = V_KEY_5;
                break;
            case KEY_NUMERIC_6:
                vkey = V_KEY_6;
                break;
            case KEY_NUMERIC_7:
                vkey = V_KEY_7;
                break;
            case KEY_NUMERIC_8:
                vkey = V_KEY_8;
                break;
            case KEY_NUMERIC_9:
                vkey = V_KEY_9;
                break;
            case KEY_POWER:
                vkey = V_KEY_POWER;
                break;
            case KEY_AUDIO:
                vkey = V_KEY_AUDIO;
                break;
            case KEY_MUTE:
                vkey = V_KEY_MUTE;
                break;
            case KEY_ZOOM:
                vkey = V_KEY_ZOOM;
                break;
            case KEY_SUBTITLE:
                vkey = V_KEY_SUB;
                break;
            case KEY_TV:
                vkey = V_KEY_TVRADIO;
                break;
            case KEY_TEXT:
                vkey = V_KEY_TEXT;
                break;
            case KEY_LIST:
                vkey = V_KEY_LIST;
                break;
            case KEY_MENU:
                vkey = V_KEY_MENU;
                break;
            case KEY_EXIT:
                vkey = V_KEY_EXIT;
                break;
            case KEY_EPG:
                vkey = V_KEY_EPG;
                break;
            case KEY_AGAIN:
                vkey = V_KEY_RECALL;
                break;
            case KEY_FAVORITES:
                vkey = V_KEY_FAV;
                break;
            case KEY_LEFTSHIFT:
                vkey = V_KEY_FB;
                break;
            case KEY_RIGHTSHIFT:
                vkey = V_KEY_FF;
                break;
            case KEY_PLAY:
                vkey = V_KEY_PLAY;
                break;
            case KEY_PAUSE:
                vkey = V_KEY_PAUSE;
                break;
            case KEY_STOP:
                vkey = V_KEY_STOP;
                break;
            case KEY_RECORD:
                vkey = V_KEY_RECORD;
                break;
            case KEY_RED:
                vkey = V_KEY_RED;
                break;
            case KEY_GREEN:
                vkey = V_KEY_GREEN;
                break;
            case KEY_YELLOW:
                vkey = V_KEY_YELLOW;
                break;
            case KEY_BLUE:
                vkey = V_KEY_BLUE;
                break;
            case KEY_HOME:
                vkey = V_KEY_HOME;
                break;
        }

    }

    return vkey;
}


static pthread_mutex_t key_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void key_mutex_lock(void)
{
    pthread_mutex_lock(&key_lock_mutex);
}

static inline void key_mutex_unlock(void)
{
    pthread_mutex_unlock(&key_lock_mutex);
}

static KEY_QUEUE_s m_key_queue;

void key_queue_init(KEY_QUEUE_s *q)
{
    q->front = q->rear = 0;
    q->size = 0;
}

bool key_queue_empty(KEY_QUEUE_s *q)
{
    if (q->size == 0)
        return true;
    else
        return false;
}

bool key_queue_write(KEY_QUEUE_s *q, KEY_MSG_s *x)
{
    if (q->size == KEY_QUEUE_MAX)
        return false;

    key_mutex_lock();

    memcpy(&(q->key_msg[q->rear]), x, sizeof(KEY_MSG_s));
    q->rear = (q->rear + 1) % KEY_QUEUE_MAX;
    q->size++;

    key_mutex_unlock();

    return true;
}

bool key_queue_read(KEY_QUEUE_s *q, KEY_MSG_s **x)
{
    if (q->size == 0)
        return false;

    *x = &(q->key_msg[q->front]);
    q->front = (q->front + 1) % KEY_QUEUE_MAX;
    q->size--;
    return true;
}

bool key_queue_head_get(KEY_QUEUE_s *q, KEY_MSG_s **x)
{
    if (q->size == 0)
        return false;

    *x = &(q->key_msg[q->front]);
    return true;
}

int key_queue_num(KEY_QUEUE_s *q)
{
    return q->size;
}


#define INTERVAL_FIRST_KEY      250 //ms, the first repeated key would be generated if long pressing
#define INTERVAL_NORMAL_KEY     100 //ms, the later repeated key would be generated if long pressing
static bool _key_event_is_valid(struct input_event *t)
{
    static uint32_t tick_last = 0;
    static uint32_t tick_intreval = 0;
    static uint32_t pressed_key = 0;
    static uint32_t key_interval = 0;
    static int first_press = 0;

    //Skip redundancy keys.
    if (t->value != 0 && t->value != 1 && t->type != EV_MSC)
        return false;
    if (t->value == 0 && t->code == 0)
        return false;

    if (t->type == EV_KEY)
    {
        if (t->value == 1) // pressed
        {
            tick_last = api_sys_tick_get();
            pressed_key = t->code;
            first_press = 1;
            tick_intreval = 0;
            return true;
        }
        else if (t->value == 0) // released
        {
            t->code = pressed_key;
            pressed_key = 0;
            first_press = 0;
            tick_intreval = 0;
            tick_last = 0;
            return true;
        }
    }
    else if (t->type == EV_MSC) // repeat key
    {
        tick_intreval = api_sys_tick_get() - tick_last;

        if (tick_intreval > 0 && pressed_key != 0)
        {
            if (first_press)
                key_interval = INTERVAL_FIRST_KEY;
            else
                key_interval = INTERVAL_NORMAL_KEY;

            if (tick_intreval > key_interval)
            {
                tick_last = api_sys_tick_get();
                t->code = pressed_key;
                t->value = 1;
                first_press = 0;
                return true;
            }
        }
    }

    return false;
}


#define INPUT_DEV_MAX   10
static volatile uint8_t m_key_off = 0;
static void *_key_task(void *arg)
{
    struct pollfd *pfd;
    int cnt;
    KEY_MSG_s key_msg;
    struct input_event *t;
    int input_cnt = 0;
    uint8_t input_name[32];
    int key_fds[INPUT_DEV_MAX] = {0,};
    int i = 0;
    char dev_name[256] = "Unknown";


    for (i = 0; i < INPUT_DEV_MAX; i++)
    {
        sprintf(input_name, "/dev/input/event%d", i);
        key_fds[input_cnt] = open(input_name, O_RDONLY);
        if (key_fds[input_cnt] < 0)
            break;

        ioctl(key_fds[input_cnt], EVIOCGNAME(sizeof(dev_name)), dev_name);
        if (strstr(dev_name, "key"))
        {
            input_cnt++;
            printf("%s %d inputdev :%s\n", __func__, __LINE__, dev_name);
        }
        else
            close(key_fds[input_cnt]);
    }
    if (0 == input_cnt)
    {
        printf("%s(), line:%d. No Input device!!!\n", __func__, __LINE__);
        return NULL;
    }

    printf("%s(), input device num: %d.\n", __func__, input_cnt);
    pfd = (struct pollfd *)malloc(sizeof(struct pollfd) * input_cnt);
    memset(pfd, 0, sizeof(struct pollfd)*input_cnt);

    for (i = 0; i < input_cnt; i ++)
    {
        pfd[i].fd = key_fds[i];
        pfd[i].events = POLLIN | POLLRDNORM;
    }

    t = &key_msg.key_event;
    while (1)
    {
        if (poll(pfd, input_cnt, -1) <= 0)
        {
            continue;
        }

        for (i = 0; i < input_cnt; i ++)
        {

            if (pfd[i].revents != 0)
            {
                cnt = read(key_fds[i], t, sizeof(struct input_event));

                if (m_key_off)
                {
                    api_sleep_ms(10);
                    continue;
                }

                if (cnt != sizeof(struct input_event))
                {
                    printf("cnt = %d, err(%d):%s\n", cnt, errno, strerror(errno));
                    api_sleep_ms(10);
                }

                if (_key_event_is_valid(t))
                {
                    //here may change the key type, for sometims
                    //event0 may be IR, may be GPIO or ADC key.
                    if (0 == i)
                        key_msg.key_type = KEY_TYPE_IR;
                    else if (1 == i)
                        key_msg.key_type = KEY_TYPE_ADC;
                    else if (2 == i)
                        key_msg.key_type = KEY_TYPE_GPIO;
                    key_queue_write(&m_key_queue, &key_msg);
                }

            }
        }
        api_sleep_ms(5);
    }
    for (i = 0; i < input_cnt; i ++)
    {
        if (key_fds[i] > 0)
            close(key_fds[i]);
    }
    free(pfd);

    return 0;
}


void api_key_get_init()
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    key_queue_init(&m_key_queue);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, _key_task, NULL))
    {
        return;
    }
    pthread_attr_destroy(&attr);
}

KEY_MSG_s *api_key_msg_get(void)
{
    KEY_MSG_s *key_msg = NULL;

    if (key_queue_read(&m_key_queue, &key_msg))
        return key_msg;
    else
        return NULL;
}

void api_key_queue_send(uint32_t act_key, int32_t press)
{
    KEY_MSG_s key_msg;
    memset(&key_msg, 0, sizeof(KEY_MSG_s));

    key_msg.key_type = KEY_TYPE_IR;
    key_msg.key_event.type = EV_KEY;
    key_msg.key_event.code = act_key;
    key_msg.key_event.value = press;//1: pressed; 0: release
    key_queue_write(&m_key_queue, &key_msg);
}


void api_key_disable(char disable)
{
    m_key_off = disable;
}

