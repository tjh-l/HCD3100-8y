// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright © 1999-2010 David Woodhouse <dwmw2@infradead.org>
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <nuttx/fs/fs.h>

#include <linux/uaccess.h>

#include "mtdcore.h"

#define get_user(val, ptr) ({val = *ptr; 0;})
#define put_user(val, ptr) ({*ptr = val; 0;})
static DEFINE_MUTEX(mtd_mutex);

/*
 * Data structure to hold the pointer to the mtd device as well
 * as mode information of various use cases.
 */
struct mtd_file_info {
	struct mtd_info *mtd;
	enum mtd_file_modes mode;
};

static int mtdchar_open(struct file *file)
{
	int ret = 0;
	struct mtd_info *mtd;
	struct mtd_file_info *mfi;
	struct inode *inode = file->f_inode;

	pr_debug("MTD_open\n");

	mutex_lock(&mtd_mutex);

	mtd = (struct mtd_info *)inode->i_private;

	if (IS_ERR(mtd)) {
		ret = PTR_ERR(mtd);
		goto out;
	}

	if (mtd->type == MTD_ABSENT) {
		ret = -ENODEV;
		goto out1;
	}

	mfi = kzalloc(sizeof(*mfi), GFP_KERNEL);
	if (!mfi) {
		ret = -ENOMEM;
		goto out1;
	}
	mfi->mtd = mtd;
	file->f_priv = mfi;
	mutex_unlock(&mtd_mutex);
	return 0;

out1:
out:
	mutex_unlock(&mtd_mutex);
	return ret;
} /* mtdchar_open */

/*====================================================================*/

static int mtdchar_close(struct file *file)
{
	struct mtd_file_info *mfi = file->f_priv;
//	struct mtd_info *mtd = mfi->mtd;

	pr_debug("MTD_close\n");

	file->f_priv = NULL;
	kfree(mfi);

	return 0;
} /* mtdchar_close */

/*
 * Copies (and truncates, if necessary) OOB layout information to the
 * deprecated layout struct, nand_ecclayout_user. This is necessary only to
 * support the deprecated API ioctl ECCGETLAYOUT while allowing all new
 * functionality to use mtd_ooblayout_ops flexibly (i.e. mtd_ooblayout_ops
 * can describe any kind of OOB layout with almost zero overhead from a
 * memory usage point of view).
 */
static int mtdchar_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct mtd_file_info *mfi = file->f_priv;
	struct mtd_info *mtd = mfi->mtd;
	void __user *argp = (void __user *)arg;
	int ret = 0;
	struct mtd_info_user info;

	pr_debug("MTD_ioctl\n");

	/*
	 * Check the file mode to require "dangerous" commands to have write
	 * permissions.
	 */
	switch (cmd) {
	/* "safe" commands */
	case MEMGETREGIONCOUNT:
	case MEMGETREGIONINFO:
	case MEMGETINFO:
	case MEMREADOOB:
	case MEMREADOOB64:
	case MEMISLOCKED:
	case MEMGETOOBSEL:
	case MEMGETBADBLOCK:
	case OTPGETREGIONCOUNT:
	case OTPGETREGIONINFO:
	case ECCGETLAYOUT:
	case ECCGETSTATS:
		break;

	/* "dangerous" commands */
	case MEMERASE:
	case MEMERASE64:
	case MEMLOCK:
	case MEMUNLOCK:
	case MEMSETBADBLOCK:
	case MEMWRITEOOB:
	case MEMWRITEOOB64:
	case MEMWRITE:
		break;

	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case MEMGETREGIONCOUNT:
		if (copy_to_user(argp, &(mtd->numeraseregions), sizeof(int)))
			return -EFAULT;
		break;

	case MEMGETREGIONINFO:
	{
		uint32_t ur_idx;
		struct mtd_erase_region_info *kr;
		struct region_info_user __user *ur = argp;

		if (get_user(ur_idx, &(ur->regionindex)))
			return -EFAULT;

		if (ur_idx >= mtd->numeraseregions)
			return -EINVAL;

		kr = &(mtd->eraseregions[ur_idx]);

		if (put_user(kr->offset, &(ur->offset))
		    || put_user(kr->erasesize, &(ur->erasesize))
		    || put_user(kr->numblocks, &(ur->numblocks)))
			return -EFAULT;

		break;
	}

	case MEMGETINFO:
		memset(&info, 0, sizeof(info));
		info.type	= mtd->type;
		info.flags	= mtd->flags;
		info.size	= mtd->size;
		info.erasesize	= mtd->erasesize;
		info.writesize	= mtd->writesize;
		info.oobsize	= mtd->oobsize;
		/* The below field is obsolete */
		info.padding	= 0;
		if (copy_to_user(argp, &info, sizeof(struct mtd_info_user)))
			return -EFAULT;
		break;

	case MEMERASE:
	case MEMERASE64:
	{
		struct erase_info *erase;

		erase = kzalloc(sizeof(struct erase_info), GFP_KERNEL);
		if (!erase)
			ret = -ENOMEM;
		else {
			if (cmd == MEMERASE64) {
				struct erase_info_user64 einfo64;

				if (copy_from_user(&einfo64, argp,
					    sizeof(struct erase_info_user64))) {
					kfree(erase);
					return -EFAULT;
				}
				erase->addr = einfo64.start;
				erase->len = einfo64.length;
			} else {
				struct erase_info_user einfo32;

				if (copy_from_user(&einfo32, argp,
					    sizeof(struct erase_info_user))) {
					kfree(erase);
					return -EFAULT;
				}
				erase->addr = einfo32.start;
				erase->len = einfo32.length;
			}

			ret = mtd_erase(mtd, erase);
			kfree(erase);
		}
		break;
	}

	case MEMLOCK:
	{
		struct erase_info_user einfo;

		if (copy_from_user(&einfo, argp, sizeof(einfo)))
			return -EFAULT;

		ret = mtd_lock(mtd, einfo.start, einfo.length);
		break;
	}

	case MEMUNLOCK:
	{
		struct erase_info_user einfo;

		if (copy_from_user(&einfo, argp, sizeof(einfo)))
			return -EFAULT;

		ret = mtd_unlock(mtd, einfo.start, einfo.length);
		break;
	}

	case MEMISLOCKED:
	{
		struct erase_info_user einfo;

		if (copy_from_user(&einfo, argp, sizeof(einfo)))
			return -EFAULT;

		ret = mtd_is_locked(mtd, einfo.start, einfo.length);
		break;
	}

	case MEMGETBADBLOCK:
	{
		loff_t offs;

		if (copy_from_user(&offs, argp, sizeof(loff_t)))
			return -EFAULT;
		return mtd_block_isbad(mtd, offs);
	}

	case MEMSETBADBLOCK:
	{
		loff_t offs;

		if (copy_from_user(&offs, argp, sizeof(loff_t)))
			return -EFAULT;
		return mtd_block_markbad(mtd, offs);
	}

	case ECCGETSTATS:
	{
		if (copy_to_user(argp, &mtd->ecc_stats,
				 sizeof(struct mtd_ecc_stats)))
			return -EFAULT;
		break;
	}

	}

	return ret;
} /* memory_ioctl */

static int mtdchar_unlocked_ioctl(struct file *file, int cmd, unsigned long arg)
{
	int ret;

	mutex_lock(&mtd_mutex);
	ret = mtdchar_ioctl(file, cmd, arg);
	mutex_unlock(&mtd_mutex);

	return ret;
}

static const struct file_operations mtd_fops = {
	.read		= dummy_read,
	.write		= dummy_write,
	.ioctl		= mtdchar_unlocked_ioctl,
	.open		= mtdchar_open,
	.close		= mtdchar_close,
};

int __init mtdchar_register(const char *devpath, struct mtd_info *priv)
{
	int ret = 0;

	ret = register_driver(devpath, &mtd_fops, 0666, priv);
	if (ret < 0) {
		pr_err("Can't register_driver %s for MTD\n", devpath);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(mtdchar_register);
