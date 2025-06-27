/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *           (c) 2011 Michael Olbrich <m.olbrich@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif
#ifdef HAVE_FIEMAP
#include <linux/fiemap.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>

#include "genimage.h"

#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT 0x800
#endif

static void *xzalloc(size_t n)
{
	void *m = malloc(n);

	if (!m) {
		printf("out of memory\n");
		return NULL;
	}

	memset(m, 0, n);

	return m;
}

static int is_block_device(const char *filename) {
	struct stat s;
	return stat(filename, &s) == 0 && ((s.st_mode & S_IFMT) == S_IFBLK);
}

static int open_file(struct image *image, const char *filename, int extra_flags)
{
	int flags = O_WRONLY | extra_flags;
	int ret, fd;

	/* make sure block devices are unused before writing */
	if (is_block_device(filename))
		flags |= O_EXCL;
	else
		flags |= O_CREAT;

	fd = open(filename, flags, 0666);
	if (fd < 0) {
		ret = -errno;
		image_error(image, "open %s: %s\n", filename, strerror(errno));
		return ret;
	}
	return fd;
}

static int insert_data(struct image *image, const void *_data, const char *outfile,
		size_t size, unsigned long long offset)
{
	if ((offset + size) > image->outbuf_size) {
		if (image->outbuf == NULL)
			image->outbuf = malloc(offset + size);
		else
			image->outbuf = realloc(image->outbuf, offset + size);
		image->outbuf_size = offset + size;
	}

	memcpy(image->outbuf + offset, _data, size);

	return 0;
}

static int extend_file(struct image *image, size_t size)
{
	if (image->outbuf_size < size) {
		if (image->outbuf == NULL)
			image->outbuf = malloc(size);
		else
			image->outbuf = realloc(image->outbuf, size);

		memset(image->outbuf + image->outbuf_size, 0, size - image->outbuf_size);
		image->outbuf_size = size;
	}
	return 0;
}

static unsigned char uuid_byte(const char *hex)
{
	char buf[3];

	buf[0] = hex[0];
	buf[1] = hex[1];
	buf[2] = 0;
	return strtoul(buf, NULL, 16);
}

static void uuid_parse(const char *str, unsigned char *uuid)
{
	uuid[0] = uuid_byte(str + 6);
	uuid[1] = uuid_byte(str + 4);
	uuid[2] = uuid_byte(str + 2);
	uuid[3] = uuid_byte(str);

	uuid[4] = uuid_byte(str + 11);
	uuid[5] = uuid_byte(str + 9);

	uuid[6] = uuid_byte(str + 16);
	uuid[7] = uuid_byte(str + 14);

	uuid[8] = uuid_byte(str + 19);
	uuid[9] = uuid_byte(str + 21);

	uuid[10] = uuid_byte(str + 24);
	uuid[11] = uuid_byte(str + 26);
	uuid[12] = uuid_byte(str + 28);
	uuid[13] = uuid_byte(str + 30);
	uuid[14] = uuid_byte(str + 32);
	uuid[15] = uuid_byte(str + 34);
}

static int uuid_validate(const char *str)
{
	int i;

	if (strlen(str) != 36)
		return -1;
	for (i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (str[i] != '-')
				return -1;
			continue;
		}
		if (!isxdigit(str[i]))
			return -1;
	}

	return 0;
}

static void xvasprintf(char **strp, const char *fmt, va_list ap)
{
	if (vasprintf(strp, fmt, ap) < 0) {
		printf("out of memory\n");
	}
}

static void xasprintf(char **strp, const char *fmt, ...)
{
	va_list args;

	va_start (args, fmt);

	xvasprintf(strp, fmt, args);

	va_end (args);
}

static char *uuid_random(void)
{
	char *uuid;

	xasprintf(&uuid, "%04lx%04lx-%04lx-%04lx-%04lx-%04lx%04lx%04lx",
		  random() & 0xffff, random() & 0xffff,
		  random() & 0xffff,
		  (random() & 0x0fff) | 0x4000,
		  (random() & 0x3fff) | 0x8000,
		  random() & 0xffff, random() & 0xffff, random() & 0xffff);

	return uuid;
}

static int block_device_size(struct image *image, const char *blkdev, unsigned long long *size)
{
	struct stat st;
	int fd, ret;
	off_t offset;

	fd = open(blkdev, O_RDONLY);
	if (fd < 0 || fstat(fd, &st) < 0) {
		ret = -errno;
		goto out;
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK) {
		ret = -EINVAL;
		goto out;
	}
	offset = lseek(fd, 0, SEEK_END);
	if (offset < 0) {
		ret = -errno;
		goto out;
	}
	*size = offset;
	ret = 0;

out:
	if (ret)
		image_error(image, "failed to determine size of block device %s: %s",
			    blkdev, strerror(-ret));
	if (fd >= 0)
		close(fd);
	return ret;
}

static int prepare_image(struct image *image, unsigned long long size)
{
	image->outbuf = malloc(size);
	memset(image->outbuf, 0, size);
	image->outbuf_size = size;
	return 0;
}

static LIST_HEAD(images);
struct image *image_get(const char *filename)
{
	struct image *image;

	list_for_each_entry(image, &images, list) {
		if (!strcmp(image->file, filename))
			return image;
	}
	return NULL;
}

