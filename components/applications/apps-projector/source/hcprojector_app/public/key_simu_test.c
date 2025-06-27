#include <pthread.h>

#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif

#include <sys/poll.h>
#include <fcntl.h>
#include "key_event.h"

#include "com_api.h"
#include "lvgl/lvgl.h"
//#include "lvgl/src/misc/lv_types.h"
#include <errno.h>

//define KEY_SIMU_TEST, to enable/disable key, or 
//send simulate key for overnight testing, disable it
//while release to customer
// #define KEY_SIMU_TEST 

#ifdef KEY_SIMU_TEST

// The following console command is to simulate send testing key.

static volatile char m_key_test_exit = 0;

typedef struct {
	uint32_t key;  //simulate the key.
	uint32_t duration; //
}KEY_TEST_GROUP_t;

/*
static KEY_TEST_GROUP_t m_key_test_group[] = 
{
	{KEY_OK, 10000},
	{KEY_EXIT, 1000},

	{KEY_UP, 1000},
	{KEY_DOWN, 1000},
	{KEY_UP, 1500},
	{KEY_DOWN, 1100},
	{KEY_UP, 1000},
	{KEY_DOWN, 2000},
	{KEY_DOWN, 2000},
	{KEY_UP, 2000},
};
*/
static KEY_TEST_GROUP_t m_key_test_group[] = 
{
	{KEY_OK, 5000},
	{KEY_EXIT, 5000},
};


static void *key_test_task(void *param)
{
	int i;
	int test_count = sizeof(m_key_test_group)/sizeof(m_key_test_group[0]);

	while(!m_key_test_exit){
		for (i = 0; i < test_count; i ++){
			api_key_queue_send(m_key_test_group[i].key, 1);
			api_sleep_ms(200);
			api_key_queue_send(m_key_test_group[i].key, 0);
			printf("key_test: %d\n", m_key_test_group[i].key);
			api_sleep_ms(m_key_test_group[i].duration);
		}

		api_sleep_ms(500);
	}

	printf("\n****** exit key test ******\n");
	return NULL;
}

static int key_test(int arc, char *argv[])
{
    if (arc == 2 && atoi(argv[1]) == 0)
    {
    	printf("****** key test stop ******\n");
    	// "key_test 0" stop
    	m_key_test_exit = 1;
    }
    else
    { // "key_test"  start

    	printf("****** key test start ******\n");

    	m_key_test_exit = 0;
	    pthread_t thread_id = 0;
	    pthread_attr_t attr;
	    pthread_attr_init(&attr);
	    pthread_attr_setstacksize(&attr, 0x2000);
	    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
	    if(pthread_create(&thread_id, &attr, key_test_task, NULL))
	    {
	        pthread_attr_destroy(&attr);
	        return -1;
	    }
	    pthread_attr_destroy(&attr);


    }
    return 0;
}


static int key_onoff(int arc, char *argv[])
{
	if (arc != 2)
		return 0;

    if (atoi(argv[1]) == 0)
    {
    	api_key_disable(1);
    	printf("****** key off ******\n");
    }
    else
    { 
    	api_key_disable(0);
    	printf("****** key on ******\n");
    }
    return 0;
}


#ifdef __HCRTOS__
#include <kernel/lib/console.h>

//Start/stop key simutlate test
//"key_test 1" start
//"key_test 0" stop
CONSOLE_CMD(key_test , NULL , key_test , CONSOLE_CMD_MODE_SELF , "simulate press key test!")

//Enable/disable key.
//key 0: key off
//key 1: key on
CONSOLE_CMD(key , NULL , key_onoff , CONSOLE_CMD_MODE_SELF , "key onoff")

#else // linux

#include <termios.h>
#include <signal.h>

#include "console.h"

static struct termios stored_settings;
static void test_exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));
    tcsetattr(0 , TCSANOW , &stored_settings);

	m_key_test_exit = 1;
    if (signo){
    	//Disable fellow only exit current console, do not exit current projector app. 
	    exit(0);
    }
}

static void *_key_console_task(void *param)
{
	struct termios new_settings;

	m_key_test_exit = 0;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, test_exit_console);
	signal(SIGINT, test_exit_console);
	signal(SIGSEGV, test_exit_console);
	signal(SIGBUS, test_exit_console);

	console_init("Projector");
	console_register_cmd(NULL, "key_test", key_test, CONSOLE_CMD_MODE_SELF, "simulate press key test!");
	console_register_cmd(NULL, "key", key_onoff, CONSOLE_CMD_MODE_SELF, "key onoff");

	console_start();
	test_exit_console(0);

}

#endif //end of #ifdef __HCRTOS__

#endif //end of #ifdef KEY_SIMU_TEST


//Entening projector console.
//"exit" to exit current projector console.
void key_simu_start(void)
{
#if defined(KEY_SIMU_TEST) && defined(__linux__)
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, _key_console_task, NULL))
    {
        pthread_attr_destroy(&attr);
        return -1;
    }
    pthread_attr_destroy(&attr);
#endif	
}
