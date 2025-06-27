#define LOG_TAG "persistmemfs"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <dirent.h>
#include <nuttx/fs/fs.h>
#include <kernel/module.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/crc32.h>
#include <kernel/elog.h>
#include <nuttx/mtd/mtd.h>
#include <linux/mutex.h>
#include <hcuapi/persistentmem.h>
#include <linux/slab.h>

#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct persistentmem_fs_device {
	char devpath[64];
	const char *mntpoint;
	struct mutex files_lock;
	void *buffer;
	int buf_size;
	int valid;
};

struct vnode {
	uint16_t id;
	uint16_t size;
};

static struct persistentmem_fs_device persistentmem_fs_dev = { 0 };
static struct persistentmem_fs_device *gfsdev = &persistentmem_fs_dev;

static int get_current_idx(void)
{
	int idx;
	size_t rc;
	char fpath[64] = { 0 };
	FILE *fp;

	snprintf(fpath, sizeof(fpath), "%s/persistentmem-idx", gfsdev->mntpoint);
	fp = fopen(fpath, "rb");
	if (!fp)
		return -1;

	rc = fread(&idx, 1, 4, fp);
	fclose(fp);
	if (rc != 4 || (idx != 0 && idx != 1))
		return -1;
	return idx;
}

static int set_current_idx(int idx)
{
	size_t rc;
	char fpath[64] = { 0 };
	FILE *fp;

	if (idx != 0 && idx != 1)
		return -1;

	snprintf(fpath, sizeof(fpath), "%s/persistentmem-idx", gfsdev->mntpoint);
	fp = fopen(fpath, "wb");
	if (!fp)
		return -1;

	rc = fwrite(&idx, 1, 4, fp);
	fflush(fp);
	fclose(fp);
	if (rc != 4)
		return -1;
	return idx;
}

static int load_persistentmem_bin(void)
{
	FILE *fp;
	struct stat sb;
	int idx = get_current_idx();
	ssize_t rc;
	char fpath[64];

	if (idx < 0)
		idx = set_current_idx(0);
	if (idx < 0)
		return -1;

	memset(fpath, 0, sizeof(fpath));
	snprintf(fpath, sizeof(fpath), "%s/persistentmem-%d.bin", gfsdev->mntpoint, idx);
	if (stat(fpath, &sb) == -1) {
		gfsdev->buf_size = 0;
		gfsdev->buffer = NULL;
		return 0;
	} else {
		gfsdev->buf_size = (int)sb.st_size;
		gfsdev->buffer = malloc(gfsdev->buf_size);
		if (!gfsdev->buffer)
			return -ENOMEM;
	}
	fp = fopen(fpath, "rb");
	rc = fread(gfsdev->buffer, 1, gfsdev->buf_size, fp);
	fclose(fp);
	return 0;
}

static void persistentmem_fs_lock(void)
{
	mutex_lock(&gfsdev->files_lock);
}

static void persistentmem_fs_unlock(void)
{
	mutex_unlock(&gfsdev->files_lock);
}

static ssize_t __persistentmem_fs_read(int offset, size_t nbytes, char *buf)
{
	size_t copied, valid_len = gfsdev->buf_size - offset;
	copied = min(valid_len, nbytes);
	if (copied > 0) {
		memcpy(buf, gfsdev->buffer + offset, copied);
	}
	return copied;
}

static ssize_t persistentmem_fs_read(struct file *file, char *buf, size_t nbytes)
{
	size_t copied, offset = file->f_pos;

	persistentmem_fs_lock();
	if (!gfsdev->valid) {
		persistentmem_fs_unlock();
		return 0;
	}
	copied = __persistentmem_fs_read(offset, nbytes, buf);
	file->f_pos += copied;
	persistentmem_fs_unlock();
	return (ssize_t)copied;
}

static ssize_t __persistentmem_fs_write(int offset, size_t nbytes, const char *buf)
{
	int new_size = offset + nbytes;
	FILE *fp;
	void *tmp;
	char fpath[64];
	int idx;

	if (new_size > gfsdev->buf_size) {
		if (!gfsdev->buffer) {
			gfsdev->buffer = malloc(new_size);
			if (!gfsdev->buffer)
				return -ENOMEM;
		} else {
			tmp = realloc(gfsdev->buffer, new_size);
			if (!tmp)
				return -ENOMEM;
			gfsdev->buffer = tmp;
		}
		memset(gfsdev->buffer + gfsdev->buf_size, 0, new_size - gfsdev->buf_size);
		gfsdev->buf_size = new_size;
	}

	idx = get_current_idx();
	if (idx < 0)
		idx = set_current_idx(0);
	if (idx < 0)
		return -EFAULT;

	idx = !idx;

	memset(fpath, 0, sizeof(fpath));
	snprintf(fpath, sizeof(fpath), "%s/persistentmem-%d.bin", gfsdev->mntpoint, idx);
	fp = fopen(fpath, "wb");
	if (!fp)
		return 0;

	memcpy(gfsdev->buffer + offset, buf, nbytes);
	fseek(fp, 0, SEEK_SET);
	fwrite(gfsdev->buffer, 1, gfsdev->buf_size, fp);
	fflush(fp);
	fclose(fp);
	set_current_idx(idx);

	return (ssize_t)nbytes;
}

static ssize_t persistentmem_fs_write(struct file *file, const char *buf, size_t nbytes)
{
	int offset = file->f_pos;
	ssize_t rc;

	persistentmem_fs_lock();
	if (!gfsdev->valid) {
		persistentmem_fs_unlock();
		return 0;
	}

	rc = __persistentmem_fs_write(offset, nbytes, buf);
	if (rc > 0)
		file->f_pos += rc;
	persistentmem_fs_unlock();

	return rc;
}

static off_t persistentmem_seek(struct file *file, off_t offset, int whence)
{
	off_t newpos;

	persistentmem_fs_lock();

	if (gfsdev->valid) {
		switch(whence) {
		case SEEK_SET:
			newpos = offset;
			break;
		case SEEK_CUR:
			newpos = file->f_pos + offset;
			break;
		case SEEK_END:
			newpos = gfsdev->buf_size + offset;
			break;
		default:
			persistentmem_fs_unlock();
			return -EINVAL;
		}

		if (newpos < 0) {
			persistentmem_fs_unlock();
			return -EINVAL;
		}

		file->f_pos = newpos;
	} else {
		persistentmem_fs_unlock();
		return -EINVAL;
	}
	persistentmem_fs_unlock();

	return newpos;
}

static void *node_find(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *p = buffer;

	if (buffer == NULL)
		return NULL;

	while (p < buffer + size) {
		pvnode = (struct vnode *)p;
		if (pvnode->id == id)
			return p;
		if (pvnode->id == PERSISTENTMEM_NODE_ID_UNUSED)
			return NULL;

		p += (sizeof(*pvnode) + pvnode->size);
	}

	return NULL;
}

static int node_create(struct persistentmem_node_create *node)
{
	void *buffer, *p;
	uint32_t size;
	struct vnode vnode;
	int len;

	buffer = gfsdev->buffer;
	size = gfsdev->buf_size;
	p = buffer;

	/* check if id duplicate */
	if (node_find(buffer, size, node->id)) {
		return PERSISTENTMEM_ERR_ID_DUPLICATED;
	}

	p = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	if (p == NULL)
		p = gfsdev->buffer + gfsdev->buf_size;

	vnode.id = node->id;
	vnode.size = node->size;
	len = __persistentmem_fs_write((int)(p - buffer), sizeof(struct vnode), (uint8_t *)&vnode);
	if (len != sizeof(struct vnode)) {
		log_e("Write error, len:%d, data_len:%ld", len, sizeof(struct vnode));
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static int node_delete(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *pcur = NULL;
	void *pnext = NULL;
	void *pend = NULL;
	int len;
	void *tmp;

	pcur = node_find(buffer, size, id);
	if (!pcur) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	pend = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	pvnode = (struct vnode *)pcur;
	pnext = pcur + sizeof(struct vnode) + pvnode->size;
	tmp = kmalloc(pend - pcur, GFP_KERNEL);
	if (!tmp) {
		return -ENOMEM;
	}

	if (pnext == pend) {
		memset(tmp, 0, pend - pcur);
	} else {
		memcpy(tmp, pnext, pend - pnext);
		memset(tmp + (pend - pnext), 0, (pnext - pcur));
	}

	len = __persistentmem_fs_write((int)(pcur - buffer), (int)(pend - pcur), (uint8_t *)tmp);
	if (len != (pend - pcur)) {
		log_e("Write error, len:%d, data_len:%ld", len, pend - pcur);
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static int node_get(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	memcpy(node->buf, (char *)pvnode + sizeof(struct vnode) + node->offset, node->size);

	return 0;
}

static int node_put(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;
	int start;
	uint32_t len;

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	start = (int)((void *)pvnode + sizeof(struct vnode) + node->offset - buffer);
	len = __persistentmem_fs_write(start, node->size, (uint8_t *)node->buf);
	if (len != node->size) {
		log_e("Write error, len:%ld, data_len:%d", len, node->size);
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static int persistentmem_fs_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case PERSISTENTMEM_IOCTL_NODE_CREATE: {
		persistentmem_fs_lock();
		if (gfsdev->valid) {
			struct persistentmem_node_create node;
			memcpy((void *)&node, (void *)arg, sizeof(node));
			node.size = ((node.size + 3) >> 2) << 2;
			rc = node_create(&node);
		} else {
			rc = -1;
		}
		persistentmem_fs_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_DELETE: {
		persistentmem_fs_lock();
		if (gfsdev->valid) {
			rc = node_delete(gfsdev->buffer, gfsdev->buf_size, arg);
		} else {
			rc = -1;
		}
		persistentmem_fs_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_GET: {
		persistentmem_fs_lock();
		if (gfsdev->valid)
			rc = node_get(gfsdev->buffer, gfsdev->buf_size, (struct persistentmem_node *)arg);
		else
			rc = -1;
		persistentmem_fs_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_PUT: {
		persistentmem_fs_lock();
		if (gfsdev->valid)
			rc = node_put(gfsdev->buffer, gfsdev->buf_size, (struct persistentmem_node *)arg);
		else
			rc = -1;
		persistentmem_fs_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_MARK_INVALID: {
		persistentmem_fs_lock();
		gfsdev->valid = 0;
		persistentmem_fs_unlock();
		break;
	}
	default:
		break;
	}

	return rc;
}

static const struct file_operations nand_persistentmem_fops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = persistentmem_fs_read, /* read */
	.write = persistentmem_fs_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = persistentmem_fs_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int persistentmem_fs_probe(const char *node)
{
	const char *mtdname = NULL;
	const char *fstype = NULL;
	const char *option = NULL;
	int np, idx, ret = 0;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	fdt_get_property_string_index(np, "mtdname", 0, &mtdname);
	if (mtdname == NULL) {
		printf("Error probe mtd name\n");
		return 0;
	}

	fdt_get_property_string_index(np, "fs-type", 0, &fstype);
	if (fstype == NULL) {
		printf("Error probe fs-type\n");
		return 0;
	}

	fdt_get_property_string_index(np, "mount-point", 0, &gfsdev->mntpoint);
	if (gfsdev->mntpoint == NULL) {
		printf("Error probe mount-point\n");
		return 0;
	}

	idx = get_mtd_device_index_nm(mtdname);
	if (idx < 0) {
		printf("Not found %s\n", mtdname);
		return 0;
	}

	snprintf(gfsdev->devpath, sizeof(gfsdev->devpath), "/dev/mtdblock%d", idx);
	if (opendir(gfsdev->mntpoint) == NULL) {
		if (!strcmp(fstype, "yaffs2"))
			option = "inband-tags,lazy-loading-on";
		else if (!strcmp(fstype, "littlefs"))
			option = NULL;
		else {
			printf("Error fs-type\n");
			return 0;
		}
		ret = mount(gfsdev->devpath, gfsdev->mntpoint, fstype, 0, option);
		if (ret < 0) {
			if (!strcmp(fstype, "yaffs2"))
				option = "forceformat,inband-tags";
			else if (!strcmp(fstype, "littlefs"))
				option = "forceformat";
			ret = mount(gfsdev->devpath, gfsdev->mntpoint, fstype, 0, option);
		}
		if (ret < 0) {
			printf("Err mount %s on %s with %s\n", gfsdev->devpath, gfsdev->mntpoint, fstype);
			return 0;
		}
	} else {
		closedir((char *)gfsdev->mntpoint);
	}

	if (load_persistentmem_bin() < 0)
		return 0;

	mutex_init(&gfsdev->files_lock);
	gfsdev->valid = 1;
	ret = register_driver("/dev/persistentmem", &nand_persistentmem_fops, 0666, NULL);
	if (ret) {
		log_e("unable to register persistentmem device\n");
		goto err_probe;
	}

	return ret;

err_probe:
	return ret;
}

static int persistentmem_fs_init(void)
{
	return persistentmem_fs_probe("/hcrtos/persistentmem");
}

static int persistentmem_fs_exit(void)
{
	unregister_driver("/dev/persistentmem");
	return 0;
}

module_system(persistentmem_fs, persistentmem_fs_init, persistentmem_fs_exit, 2)
