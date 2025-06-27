#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <hcuapi/glock.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>

static int glock_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	SemaphoreHandle_t lock = (SemaphoreHandle_t)inode->i_private;
	int rc = 0;

	if (!lock)
		return -EFAULT;

	switch (cmd) {
	case GLOCKIOC_SET_LOCKTIMEOUT:
		if (xSemaphoreTake(lock, (TickType_t)arg) == pdTRUE)
			rc = 0;
		else
			rc = -ETIMEDOUT;
		break;
	case GLOCKIOC_SET_LOCK:
		xSemaphoreTake(lock, portMAX_DELAY);
		break;
	case GLOCKIOC_SET_UNLOCK:
		xSemaphoreGive(lock);
		break;
	case GLOCKIOC_SET_TRYLOCK:
		rc = (xSemaphoreTake(lock, 0) == pdTRUE);
		break;
	default:
		rc = -ENOTSUP;
		break;
	}

	return rc;
}

static const struct file_operations glock_fops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = glock_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static void glock_probe(const char *node)
{
	int np;
	int i;
	char strbuf[128];
	const char *name;
	SemaphoreHandle_t lock;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return;

	for (i = 0;;i++) {
		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "glock-%d", i);
		if (fdt_get_property_string_index(np, strbuf, 0, &name))
			break;

		lock = xSemaphoreCreateMutex();
		if (!lock) {
			printf("Error create global lock\r\n");
			break;
		}

		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "/dev/%s", name);
		register_driver(strbuf, &glock_fops, 0666, (void *)lock);
	}

	return;
}

static int glock_module_init(void)
{
	glock_probe("/hcrtos/global-lock");
	return 0;
}

module_arch(glock, glock_module_init, NULL, 0)
