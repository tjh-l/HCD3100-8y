/*
 * iperf, Copyright (c) 2014, 2015, 2017, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
#include <types.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "iperf.h"
#include "iperf_api.h"
#include "units.h"
#include "iperf_locale.h"
#include "net.h"

#include <kernel/lib/console.h>


static int run(struct iperf_test *test);


/**************************************************************************/
#ifndef __TR_SW__
int
main(int argc, char **argv)
#else
int
iperf3_main(int argc , char *argv[])
#endif
{
    struct iperf_test *test;
#ifdef __TR_SW__
    int ret = 0;
#endif

    // XXX: Setting the process affinity requires root on most systems.
    //      Is this a feature we really need?
#ifdef TEST_PROC_AFFINITY
    /* didnt seem to work.... */
    /*
     * increasing the priority of the process to minimise packet generation
     * delay
     */
    int rc = setpriority(PRIO_PROCESS, 0, -15);

    if (rc < 0) {
        perror("setpriority:");
        fprintf(stderr, "setting priority to valid level\n");
        rc = setpriority(PRIO_PROCESS, 0, 0);
    }
    
    /* setting the affinity of the process  */
    cpu_set_t cpu_set;
    int affinity = -1;
    int ncores = 1;

    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);
    if (errno)
        perror("couldn't get affinity:");

    if ((ncores = sysconf(_SC_NPROCESSORS_CONF)) <= 0)
        err("sysconf: couldn't get _SC_NPROCESSORS_CONF");

    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0)
        err("couldn't change CPU affinity");
#endif

    test = iperf_new_test();
    if (!test)
        iperf_errexit(NULL, "create new test error - %s", iperf_strerror(i_errno));
    iperf_defaults(test);	/* sets defaults */

    if ((ret = iperf_parse_arguments(test, argc, argv)) < 0) {
#ifdef __TR_SW__
        if (ret != -2)
#endif
        iperf_err(test, "parameter error - %s", iperf_strerror(i_errno));
#ifndef __TR_SW__
        fprintf(stderr, "\n");
        usage_long(stdout);
        exit(1);
#else
        if (ret == -2) {
            iperf_free_test(test);
            return 0;
        } else {
            usage_long();
        }
#endif
    }

    if (run(test) < 0) {
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
		#ifdef __TR_SW__
		ret = -1;
		#endif
    }

    iperf_free_test(test);

#ifndef __TR_SW__
    return 0;
#else
    return ret;
#endif
}

#ifndef __TR_SW__
static jmp_buf sigend_jmp_buf;

static void __attribute__ ((noreturn))
sigend_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}
#endif

/**************************************************************************/
static int
run(struct iperf_test *test)
{
#ifndef __TR_SW__
    /* Termination signals. */
    iperf_catch_sigend(sigend_handler);
    if (setjmp(sigend_jmp_buf))
	iperf_got_sigend(test);

    /* Ignore SIGPIPE to simplify error handling */
    signal(SIGPIPE, SIG_IGN);
#endif

    switch (test->role) {
        case 's':
#ifndef __TR_SW__            
	    if (test->daemon) {
		int rc;
		rc = daemon(0, 0);
		if (rc < 0) {
		    i_errno = IEDAEMON;
		    iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
		}
	    }
	    if (iperf_create_pidfile(test) < 0) {
		i_errno = IEPIDFILE;
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    }
#endif
        for (;;) {
		int rc;
		rc = iperf_run_server(test);
		if (rc < 0) {
#ifdef __TR_SW__
		    {
		        struct iperf_stream *sp;

		        /* Close all stream sockets */
		        SLIST_FOREACH(sp, &test->streams, streams) {
			    close(sp->socket);
		        }
		    }
#endif
		    iperf_err(test, "error - %s", iperf_strerror(i_errno));
		    if (rc < -1) {
		        iperf_errexit(test, "exiting");
#ifdef __TR_SW__
			break;
#endif
		    }
                }
                iperf_reset_test(test);
                if (iperf_get_test_one_off(test))
                    break;
#ifdef __TR_SW__
		if (i_errno == IESERVERTERM) {
		    i_errno = IENONE;
		    break;
		}
#endif
            }
#ifndef __TR_SW__
	    iperf_delete_pidfile(test);
#endif
            break;
	case 'c':
	    if (iperf_run_client(test) < 0) {
#ifdef __TR_SW__
	        struct iperf_stream *sp;

		    /* Close all stream sockets */
		    SLIST_FOREACH(sp, &test->streams, streams) {
		        close(sp->socket);
		    }
#endif

			iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    }
            break;
        default:
            usage();
            break;
    }

#ifndef __TR_SW__
    iperf_catch_sigend(SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
#endif

    return 0;
}

#ifdef __TR_SW__
iperf3_test_status_e iperf3_run_status = IPERF3_STATUS_IDLE;
static unsigned int iperf_task_handle = NULL;
char ipef3_clibuf[512] = {0};

static int isblank2(char c)
{
    return (( c == ' ' ) || ( c == '\t' ));
}

#define IPERF3_MAX_ARGC 16
static int iperf3_cmd_parse_line(char *s, char *argv[])
{
	int argc = 0;

	while (argc < IPERF3_MAX_ARGC) {
		while (isblank2(*s))
			s++;

		if (*s == '\0')
			goto out;

		argv[argc++] = s;

		while (*s && !isblank2(*s))
			s++;

		if (*s == '\0')
			goto out;

		*s++ = '\0';
	}

	printf("Too many args\n");

 out:
	argv[argc] = NULL;
	return argc;
}

int is_iperf3_stop_request(void)
{
    return IPERF3_STATUS_STOPPING == iperf3_run_status;
}

int set_iperf3_run_status(iperf3_test_status_e status)
{
    if (status < IPERF3_STATUS_MAX)
        iperf3_run_status = status;

    return 0;
}

int get_iperf3_run_status(void)
{
    return iperf3_run_status;
}

void set_iperf3_args(const char *buf)
{
    snprintf(ipef3_clibuf, sizeof(ipef3_clibuf), "%s", buf);
}

static void iperf3_task(void *para1)
{
    struct iperf_test *test = (struct iperf_test *)para1;
    run(test);

    printf("iperf(0x%x) %c port %d stop\n",  test->taskid, test->role, test->server_port);
    test->freed = 1;
	vTaskDelete(NULL);
}

#define IPERF_LINK_MAX_NUM  16
#define IPERF_LINK_SERVER   4
#define IPERF_LINK_CLIENT   16
#define	CMD_RET_SUCCESS     0
#define	CMD_RET_FAILURE     1

struct iperf_test *iperf_thread[IPERF_LINK_MAX_NUM];

static int iperf_stop_server(int port)
{
    int index;
    struct iperf_test *server;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        server = iperf_thread[index];
        if (server == NULL || server->freed == 1)
            continue;

        if (server->role == 's' && server->server_port == port) {
            server->state = IPERF_DONE;
            server->one_off = 1;
            return CMD_RET_SUCCESS;
        }
    }

    printf("server not start with port %d\n", port);
    return CMD_RET_FAILURE;
}

static int iperf_stop_client(char *ip, int port)
{
    int index;
    struct iperf_test *client;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        client = iperf_thread[index];
        if (client == NULL || client->freed == 1)
            continue;

        if (client->role == 'c' && client->server_port == port && !strcmp(client->server_hostname, ip)) {
            if (client->state != IPERF_DONE) {
                if (client->state == TEST_RUNNING) {
                    if (iperf_set_send_state(client, TEST_END) != 0)
                        return CMD_RET_FAILURE;
                }
            }
            return CMD_RET_SUCCESS;
        }
    }

    printf("client not start with ip %s port %d\n", ip, port);
    return CMD_RET_FAILURE;
}

int iperf_sc_check(struct iperf_test *sc)
{
    int index;
    int tcpnum = 0;
    int udpnum = 0;
    int snum = 0;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        if (iperf_thread[index] == NULL || iperf_thread[index]->freed)
            continue;

        if (sc->role == 'c')
        {
            if (sc != iperf_thread[index] && sc->server_port == iperf_thread[index]->server_port && !strcmp(sc->server_hostname, iperf_thread[index]->server_hostname))
            {
                printf("client donot start same ip %s port %d\n", sc->server_hostname, sc->server_port);
                return CMD_RET_FAILURE;
            }
        }

        if (sc->role == 's')
        {
            if (sc != iperf_thread[index] && iperf_thread[index]->role == 's' && sc->server_port == iperf_thread[index]->server_port)
            {
                printf("server donot start same port %d\n", sc->server_port);
                return CMD_RET_FAILURE;
            }
        }
        //printf("index %d server->role %c ip %s port=%d state=%d snum=%d\n", index, iperf_thread[index]->role, iperf_thread[index]->server_hostname, iperf_thread[index]->server_port, iperf_thread[index]->state, iperf_thread[index]->num_streams);

        if (iperf_thread[index]->role == 's')
        {
            if ((sc->state > 0) && sc->state != IPERF_START)
            {
                tcpnum += 1;
            }
            snum += 1;
            if (snum > IPERF_LINK_SERVER)
            {
                printf("server max link, please check...\n");
                return CMD_RET_FAILURE;
            }
        }
        else
        {
            tcpnum += 1;
        }

        if (iperf_thread[index]->protocol->id == SOCK_DGRAM)
        {
            udpnum += iperf_thread[index]->num_streams;
        }
        else
        {
            tcpnum += iperf_thread[index]->num_streams;
        }

        if (udpnum + tcpnum > IPERF_LINK_CLIENT || udpnum > IPERF_LINK_CLIENT/2)
        {
            printf("iperf max link, please check...\n");
            return CMD_RET_FAILURE;
        }
    }

    return CMD_RET_SUCCESS;
}

static int iperf_test(int argc,char *argv[])
{
    int index;
    int port = PORT;
	BaseType_t ret = pdFAIL;

    if (argc <= 1) {
        return CMD_RET_FAILURE;
    }

    if (strcmp(argv[1], "stop") == 0)
    {
        //iperf stop port xxx
        if (argv[2]&&strcmp(argv[2], "port") == 0)
        {
            if (argc == 4)
                port = atoi(argv[3]);
            return iperf_stop_server(port);
        }

		//iperf stop ip port xxx
        if (argv[2]&&strcmp(argv[2], "ip") == 0)
        {
            if (strcmp(argv[4], "port") == 0)
            {
                port = atoi(argv[5]);
            }
            return iperf_stop_client(argv[3], port);
        }
		
//        os_printf(LM_CMD, LL_INFO, "iperf stop port[ip] xxx\n");
        return CMD_RET_FAILURE;
    }

    if (argc == 2 && !strcmp(argv[1], "-h"))
    {
        iperf3_main(argc, argv);
        return CMD_RET_SUCCESS;
    }

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        if (iperf_thread[index] == NULL || iperf_thread[index]->freed == 1)
        {
            if (iperf_thread[index]) {
                iperf_free_test(iperf_thread[index]);
                iperf_thread[index] = NULL;
            }
            iperf_thread[index] = iperf_new_test();
            if (!iperf_thread[index])
            {
                printf("iperf_new_test failed\n");
                return CMD_RET_FAILURE;
            }

            iperf_defaults(iperf_thread[index]);
            iperf_thread[index]->freed = 0;
            if (iperf_parse_arguments(iperf_thread[index], argc, argv) < 0)
            {
                printf("iperf_parse_arguments failed\n");
                iperf_free_test(iperf_thread[index]);
                iperf_thread[index] = NULL;
                return CMD_RET_SUCCESS;
            }

            if (!iperf_sc_check(iperf_thread[index]))
            {
				ret = xTaskCreate(iperf3_task,"iperf3", 8*1024,
					  (void*)iperf_thread[index], portPRI_TASK_NORMAL, iperf_thread[index]->taskid);
                printf("tsk_id = %d\n", iperf_thread[index]->taskid);

                return CMD_RET_SUCCESS;
            }

            iperf_free_test(iperf_thread[index]);
            iperf_thread[index] = NULL;
            return CMD_RET_FAILURE;
        }
    }

    printf("max link start\n");
    return CMD_RET_FAILURE;
}

CONSOLE_CMD(iperf3,NULL, iperf_test, CONSOLE_CMD_MODE_SELF, "iperf3")

#endif
