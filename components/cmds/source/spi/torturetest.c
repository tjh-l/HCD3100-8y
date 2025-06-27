#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/spidev.h>
#include <kernel/lib/console.h>
#include <sys/time.h>
#include <nuttx/mtd/mtd.h>
#include <linux/mtd/mtd-abi.h>

static char mtd_path[128];
static char mtdname[64];
static struct mtd_geometry_s geo = { 0 };
static uint32_t test_len = 0x100000;

static void print_help(void)
{
	printf("***********************************\n");
	printf("torturetest test cmds help\n");
	printf("\tfor example : torturetest -d2 -c100\n");
	printf("\t'd'	2 means mtdblock2\n");
	printf("\t'c'	100 means test 100 times, defualt 10000\n");
	printf("***********************************\n");
}

static int test_read(char *buf)
{
	int fb;
	struct timeval t1, t2, tresult;
	double timeuse;
	double timeu[2] = { 0 };

	fb = open(mtd_path, O_SYNC | O_RDWR);
	if (fb < 0) {
		printf("can't open device\n");
		return -1;
	}
	//read
	gettimeofday(&t1, NULL);
	read(fb, buf, test_len);
	close(fb);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &tresult);

	timeuse =
		tresult.tv_sec + (1.0 * tresult.tv_usec) / 1000000; //  精确到秒

	timeuse = tresult.tv_sec * 1000 +
		  (1.0 * tresult.tv_usec) / 1000; //  精确到毫秒

	timeu[0] += timeuse;

	printf("read  size:0x%x use time: %fms\n", test_len, timeu[0]);
}

static int test_write(char *buf)
{
	int fb;
	struct timeval t1, t2, tresult;
	double timeuse;
	double timeu[2] = { 0 };

	fb = open(mtd_path, O_SYNC | O_RDWR);
	if (fb < 0) {
		printf("can't open device\n");
		return -1;
	}
	//write
	gettimeofday(&t1, NULL);
	write(fb, buf, test_len);
	close(fb);
	gettimeofday(&t2, NULL);
	timersub(&t2, &t1, &tresult);

	timeuse =
		tresult.tv_sec + (1.0 * tresult.tv_usec) / 1000000; //  精确到秒

	timeuse = tresult.tv_sec * 1000 +
		  (1.0 * tresult.tv_usec) / 1000; //  精确到毫秒

	timeu[0] += timeuse;

	printf("write size:0x%x use time: %fms\n", test_len, timeu[0]);
}

static void mtd_test(uint32_t counts)
{
	int fd = -1, err = -1;
	uint32_t i = 0, times = 0;

	fd = open(mtd_path, O_SYNC | O_RDWR);
	if (fd < 0) {
		return ;
	}
	err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
	if (err < 0) {
		close(fd);
		return ;
	}

	test_len = geo.erasesize * geo.neraseblocks;

	char *buf_after = malloc(test_len);
	char *buf_test_data = malloc(test_len);
	if (buf_after == NULL || buf_test_data == NULL) {
		printf("malloc error\n");
		return ;
	}
	memset(buf_after, 0, sizeof(buf_after));
	memset(buf_test_data, 0, sizeof(buf_test_data));

	printf("Test len = 0x%lx\n", test_len);

	srand(time(NULL));
	for (times = 0; times < counts; times++) {
		printf("============================================\n");
		/* get random data */
		for (i = 0; i < test_len; i++) {
			buf_test_data[i] = rand() % test_len;
		}
		/* write */
		test_write(buf_test_data);
		/* read */
		test_read(buf_after);
		for (i = 0; i < test_len; i++) {
			if (buf_test_data[i] != buf_after[i])
				printf("test_data[0x%lx] = %d,read_data= 0x%lx\n",
				       i, buf_test_data[i], buf_after[i]);
		}
		printf("finsh %d/%d times\n", times + 1, counts);
	}
	if (buf_after != NULL)
		free(buf_after);
	if (buf_test_data != NULL)
		free(buf_test_data);
}

static int torture_test(int argc, char **argv)
{
	int dev_num = 0;
	uint32_t counts = 10000;

	char ch;
	long tmp;
	int fd = -1;
	opterr = 0;
	optind = 0;

	while((ch = getopt(argc, argv, "hd:c:")) != EOF){
		switch (ch) {
			case 'h':
				print_help();
				return 0;
			case 'd':
				tmp = strtoll(optarg, NULL,10);
				dev_num = tmp;
				break;
			case 'c':
				tmp = strtoll(optarg, NULL,10);
				counts = tmp;
				break;
			default:
				printf("Invalid parameter %c\r\n", ch);
				print_help();
				return -1;
		}
	}

	if (dev_num == 0) {
		print_help();
		return -1;
	}

	sprintf(mtdname, "mtdblock%d", dev_num);
	sprintf(mtd_path, "/dev/mtdblock%d", dev_num);

	fd = open(mtd_path, O_RDWR);
	if (fd < 0) {
		printf("open %s fail\n", mtd_path);
		return -1;
	}
	close(fd);

	printf("%s Will Test %d times...\n", mtd_path, counts);

	mtd_test(counts);

	printf("Finsh test\n");

	return 0;
}

CONSOLE_CMD(torturetest, "mtd_test", torture_test, CONSOLE_CMD_MODE_SELF, "will circular erase reading and writing whole mtdblock ")
