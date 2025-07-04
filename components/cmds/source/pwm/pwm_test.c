#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <hcuapi/pwm.h>
#include <kernel/lib/console.h>

/* About How to calculated PWM frequency
 * freq = 27M / (div * (h_val + l_val));
 * h_val = duty_ns / div /37;
 * l_val = (period_ns - duty_ns) / div /37;
 * To meet the adjustable level of 1%,
 * the h_val and l_val have following requirements:
 *      Max(h_val + l_val) = 65535;
 *      Min(h_val + l_val) = 100;
 * the div can set in make menuconfig:
 *      CONFIG_RANGE_411Hz_270KHz: div = 1;
 *      CONFIG_RANGE_206Hz_135KHz: div = 2;
 *      CONFIG_RANGE_103Hz_67KHz : div = 4;
 *      CONFIG_RANGE_50Hz_33KHz  : div = 8;
 *      CONFIG_RANGE_AUTO_SET    : when use only one channel ,
 *                                 div will auto calculated to meet with the frequency you set;
 * if div = 1; the rang is 411(Hz)-270(KHz),
 * you can set bigger than 270(KHz), but adjustable level may not be 1%;
 */

static void print_usage(const char *prog)
{
        printf("Usage: %s \n", prog);
        puts("  -s --start     		start pwm device id\n"
	     "  -p --period_ns  	set period_ns\n"
	     "  -d --duty_ns		set duty_ns\n"
	     "  -P --ploarity		set ploarity\n"
	     "  -S --stop		stop pwm device id\n");
}

static int pwm_test(int argc, char *argv[])
{
	int fd;
	int id = 0;
	bool stop_t = false;
	char path[64] = { 0 };
	struct pwm_info_s info = { 0 };
	uint32_t period_ns, duty_ns, polarity;
	period_ns = 1000000;
	duty_ns = 500000;
	polarity = 0;

	opterr = 0;
	optind = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "start",         1, 0, 's' },
			{ "period_ns",     1, 0, 'p' },
			{ "duty_ns",       1, 0, 'd' },
			{ "stop",          1, 0, 'S' },
			{ "polarity",      1, 0, 'P' },
			{ "NULL",   	0, 0, 0 },
		};

		int c;

		c = getopt_long(argc, argv, "s:p:d:S:P:", lopts, NULL);

		if (c == -1) {
			break;
		}

		switch(c) {
		case 's':
			id = atoi(optarg);
			break;
		case 'p':
			period_ns = atoi(optarg);
			break;
		case 'P':
			polarity = atoi(optarg);
			break;
		case 'd':
			duty_ns = atoi(optarg);
			break;
		case 'S':
			stop_t = true;
			id = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	sprintf(path, "/dev/pwm%d", id);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		printf("%s open fail\n", path);
		return 0;
	}

	if (stop_t == true) {
		printf("stop %s test\n", path);
		ioctl(fd, PWMIOC_STOP);
		close(fd);
		return 0;
	}

	info.polarity = polarity;
	info.period_ns = period_ns;
	info.duty_ns = duty_ns;
	ioctl(fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)&info);

	ioctl(fd, PWMIOC_START);
	close(fd);
	printf("start %s test\n", path);

	return 0;
}

CONSOLE_CMD(pwm_test, NULL, pwm_test, CONSOLE_CMD_MODE_SELF, "pwm test operations")
