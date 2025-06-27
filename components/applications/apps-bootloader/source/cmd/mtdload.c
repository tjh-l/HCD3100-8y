#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/mtd/mtd.h>
#include <freertos/FreeRTOS.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>
#include <image.h>

int mtdloadraw(int argc, char **argv)
{
	off_t seekpos;
	ssize_t nbytes;
	int fd;
	int ret;
	const char *charname = argv[2];
	void *addr = NULL;
	void *alloc = NULL;
	ssize_t length;
	struct stat sb;

	if (argc < 4) {
		printf("ERROR arguments. Usage: <addr> </dev/mtdblockX> <size>\n");
		return -1;
	}

	if (stat(charname, &sb) == -1) {
		printf("%s stat failed\n", charname);
		return -ENOENT;
	}

	length = (ssize_t)strtoul(argv[3], NULL, 0);
	if (length == -1)
		length = (ssize_t)sb.st_size;
	if (!length) {
		printf("ERROR: <size>\n");
		return -1;
	}
	length = MIN(length, (ssize_t)sb.st_size);

	addr = (void *)strtoul(argv[1], NULL, 0);
	if (addr == (void *)(-1)) {
		alloc = addr = malloc(length);
	}
	if (!addr) {
		printf("ERROR: <addr>\n");
		return -1;
	}

	fd = open(charname, O_RDONLY);
	if (fd < 0) {
		printf("ERROR: open %s\n", charname);
		goto err_release_fp;
	}

	seekpos = lseek(fd, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	nbytes = read(fd, addr, length);
	if (nbytes < 0) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	} else if (nbytes == 0) {
		printf("ERROR: Unexpected end-of file in %s\n", charname);
		ret = -1;
		goto err_release_fp;
	} else if (nbytes != length) {
		printf("ERROR: Unexpected read size from %s: %ld\n", charname,
		       (unsigned long)nbytes);
		ret = -1;
		goto err_release_fp;
	}

	printf("total %ld bytes read to %p\n", length, addr);

	raw_load_addr = (unsigned long)addr;
	raw_load_size = length;

	close(fd);
	return 0;

err_release_fp:
	if (alloc)
		free(alloc);
	if (fd >= 0)
		close(fd);
	return ret;
}

int mtdloaduImage(int argc, char **argv)
{
	const char *charname = argv[1];
	image_header_t hdr;
	off_t seekpos;
	ssize_t nbytes;
	ssize_t length = 0;
	int fd;
	int ret;

	if (argc < 2) {
		printf("ERROR arguments. Usage: </dev/mtdblockX>\n");
		return -1;
	}

	fd = open(charname, O_RDONLY);
	if (fd < 0) {
		printf("ERROR: open %s\n", charname);
		return -1;
	}

	seekpos = lseek(fd, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	length = image_get_header_size();
	nbytes = read(fd, &hdr, length);
	if (nbytes != length) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	}

	length = image_get_image_size(&hdr);
	if ((void *)image_load_addr == NULL)
		image_load_addr = (unsigned long)malloc(length);
	else
		image_load_addr = (unsigned long)realloc((void *)image_load_addr, length);
	if (!image_load_addr) {
		printf("ERROR: malloc for image with size %ld\n", length);
		ret = -1;
		goto err_release_fp;
	}
	printf("default image load address = 0x%08lx\n", image_load_addr);

	seekpos = lseek(fd, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	nbytes = read(fd, (void *)image_load_addr, length);
	if (nbytes != length) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	}

	close(fd);
	return 0;

err_release_fp:
	close(fd);
	return ret;
}

CONSOLE_CMD(mtdloadraw, NULL, mtdloadraw, CONSOLE_CMD_MODE_SELF, "<addr> </dev/mtdblockX> <size> load raw data from /dev/mtdblockX")
CONSOLE_CMD(mtdloaduImage, NULL, mtdloaduImage, CONSOLE_CMD_MODE_SELF, "</dev/mtdblockX> load uImage from /dev/mtdblockX")
