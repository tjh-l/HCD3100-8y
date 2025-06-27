/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright © 1999-2010 David Woodhouse <dwmw2@infradead.org> et al.
 */

#ifndef __MTD_MTD_H__
#define __MTD_MTD_H__

#include <linux/types.h>
#include <kernel/list.h>

#include <linux/mutex.h>
#include <nuttx/config.h>

#include <sys/types.h>

#include <hcuapi/iocbase.h>
#include <linux/errno.h>
#include <uapi/mtd/mtd-abi.h>

#include <asm-generic/div64.h>

/**
 * struct mtd_part - MTD partition specific fields
 *
 * @node: list node used to add an MTD partition to the parent partition list
 * @offset: offset of the partition relatively to the parent offset
 * @size: partition size. Should be equal to mtd->size unless
 *        MTD_SLC_ON_MLC_EMULATION is set
 * @flags: original flags (before the mtdpart logic decided to tweak them based
 *         on flash constraints, like eraseblock/pagesize alignment)
 *
 * This struct is embedded in mtd_info and contains partition-specific
 * properties/fields.
 */
struct mtd_part {
	struct list_head node;
	u64 offset;
	u64 size;
	u32 flags;
};

/**
 * struct mtd_master - MTD master specific fields
 *
 * @partitions_lock: lock protecting accesses to the partition list. Protects
 *                   not only the master partition list, but also all
 *                   sub-partitions.
 * @suspended: et to 1 when the device is suspended, 0 otherwise
 *
 * This struct is embedded in mtd_info and contains master-specific
 * properties/fields. The master is the root MTD device from the MTD partition
 * point of view.
 */
struct mtd_master {
	struct mutex partitions_lock;
	unsigned int suspended : 1;
};

#define MAX_MTD_DEVICES 32
#define MTD_ERASE_PENDING       0x01
#define MTD_ERASING             0x02
#define MTD_ERASE_SUSPEND       0x04
#define MTD_ERASE_DONE          0x08
#define MTD_ERASE_FAILED        0x10

#define MTD_FAIL_ADDR_UNKNOWN -1LL

struct erase_info {
	struct mtd_info *mtd;
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
	u_long time;
	u_long retries;
	unsigned dev;
	unsigned cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
};

struct mtd_erase_region_info {
	uint64_t offset;
	uint32_t erasesize;
	uint32_t numblocks;
	unsigned long *lockmap;
};

struct mtd_oob_ops {
	unsigned int    mode;
	size_t          len;
	size_t          retlen;
	size_t          ooblen;
	size_t          oobretlen;
	uint32_t        ooboffs;
	uint8_t         *datbuf;
	uint8_t         *oobbuf;
};

#define MTD_MAX_OOBFREE_ENTRIES_LARGE   32
#define MTD_MAX_ECCPOS_ENTRIES_LARGE    640

struct nand_ecclayout {
	__u32 eccbytes;
	__u32 eccpos[MTD_MAX_ECCPOS_ENTRIES_LARGE];
	__u32 oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES_LARGE];
};

struct mtd_info {
	u_char type;
	uint32_t flags;
	uint64_t size;   // Total size of the MTD

	/* "Major" erase size for the device. Naïve users may take this
	 * to be the only erase size available, or may use the more detailed
	 * information below if they desire
	 */
	uint32_t erasesize;
	uint32_t erasesize_se;
	/* Minimal writable flash unit size. In case of NOR flash it is 1 (even
	 * though individual bits can be cleared), in case of NAND flash it is
	 * one NAND page (or half, or one-fourths of it), in case of ECC-ed NOR
	 * it is of ECC block size, etc. It is illegal to have writesize = 0.
	 * Any driver registering a struct mtd_info must ensure a writesize of
	 * 1 or larger.
	 */
	uint32_t writesize;

	/*
	 * Size of the write buffer used by the MTD. MTD devices having a write
	 * buffer can write multiple writesize chunks at a time. E.g. while
	 * writing 4 * writesize bytes to a device with 2 * writesize bytes
	 * buffer the MTD driver can (but doesn't have to) do 2 writesize
	 * operations, but not 4. Currently, all NANDs have writebufsize
	 * equivalent to writesize (NAND page size). Some NOR flashes do have
	 * writebufsize greater than writesize.
	 */
	uint32_t writebufsize;

	uint32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	uint32_t oobavail;  // Available OOB bytes per block

	/*
	 * If erasesize is a power of 2 then the shift is stored in
	 * erasesize_shift otherwise erasesize_shift is zero. Ditto writesize.
	 */
	unsigned int erasesize_shift;
	unsigned int writesize_shift;
	/* Masks based on erasesize_shift and writesize_shift */
	unsigned int erasesize_mask;
	unsigned int writesize_mask;

	/*
	 * read ops return -EUCLEAN if max number of bitflips corrected on any
	 * one region comprising an ecc step equals or exceeds this value.
	 * Settable by driver, else defaults to ecc_strength.  User can override
	 * in sysfs.  N.B. The meaning of the -EUCLEAN return code has changed;
	 * see Documentation/ABI/testing/sysfs-class-mtd for more detail.
	 */
	unsigned int bitflip_threshold;

	// Kernel-only stuff starts here.
	const char *name;
	int index;

	/* ECC layout structure pointer - read only! */
	struct nand_ecclayout *ecclayout;

	/* the ecc step size. */
	unsigned int ecc_step_size;

	/* max number of correctible bit errors per ecc step */
	unsigned int ecc_strength;

	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above.
	 */
	int numeraseregions;
	struct mtd_erase_region_info *eraseregions;

	/*
	 * Do not call via these pointers, use corresponding mtd_*()
	 * wrappers instead.
	 */
	int (*_erase) (struct mtd_info *mtd, struct erase_info *instr);
	int (*_point) (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, void **virt, resource_size_t *phys);
	int (*_unpoint) (struct mtd_info *mtd, loff_t from, size_t len);
	unsigned long (*_get_unmapped_area) (struct mtd_info *mtd,
			unsigned long len,
			unsigned long offset,
			unsigned long flags);
	int (*_read) (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf);
	int (*_write) (struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf);
	int (*_panic_write) (struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf);
	int (*_read_oob) (struct mtd_info *mtd, loff_t from,
			struct mtd_oob_ops *ops);
	int (*_write_oob) (struct mtd_info *mtd, loff_t to,
			struct mtd_oob_ops *ops);
	void (*_sync) (struct mtd_info *mtd);
	int (*_lock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*_unlock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*_is_locked) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*_block_isreserved) (struct mtd_info *mtd, loff_t ofs);
	int (*_block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*_block_markbad) (struct mtd_info *mtd, loff_t ofs);
	int (*_suspend) (struct mtd_info *mtd);
	void (*_resume) (struct mtd_info *mtd);
	void (*_reboot) (struct mtd_info *mtd);
	/*
	 * If the driver is something smart, like UBI, it may need to maintain
	 * its own reference counting. The below functions are only for driver.
	 */
	int (*_get_device) (struct mtd_info *mtd);
	void (*_put_device) (struct mtd_info *mtd);

	/* Backing device capabilities for this device
	 * - provides mmap capabilities
	 */
	struct backing_dev_info *backing_dev_info;

	/* ECC status information */
	struct mtd_ecc_stats ecc_stats;
	/* Subpage shift (NAND) */
	int subpage_sft;

	int usecount;
	void *priv;

	/*
	 * Parent device from the MTD partition point of view.
	 *
	 * MTD masters do not have any parent, MTD partitions do. The parent
	 * MTD device can itself be a partition.
	 */
	struct mtd_info *parent;

	union {
		struct mtd_part part;
		struct mtd_master master;
	};
};

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr);
int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
		u_char *buf);
int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
		const u_char *buf);
int mtd_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static inline int mtd_write_oob(struct mtd_info *mtd, loff_t to,
		struct mtd_oob_ops *ops)
{
	ops->retlen = ops->oobretlen = 0;
	if (!mtd->_write_oob)
		return -EOPNOTSUPP;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	return mtd->_write_oob(mtd, to, ops);
}
int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs);
int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs);
#ifdef CONFIG_MTD_PROTECT
int mtd_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len);
int mtd_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len);
int mtd_is_locked(struct mtd_info *mtd, loff_t ofs, uint64_t len);
#endif

static inline int mtd_is_bitflip(int err) {
	return err == -EUCLEAN;
}

static inline int mtd_is_eccerr(int err) {
	return err == -EBADMSG;
}

static inline int mtd_is_bitflip_or_eccerr(int err) {
	return mtd_is_bitflip(err) || mtd_is_eccerr(err);
}

static inline void mtd_erase_callback(struct erase_info *instr) {
}

static inline int mtd_can_have_bb(const struct mtd_info *mtd)
{
	return !!mtd->_block_isbad;
}

int mtd_device_register(struct mtd_info *mtd, const char *node);
struct mtd_dev_s *get_mtd_device_nm(const char *name);
struct mtd_dev_s *get_mtd_device(int mtdnr);

static inline struct mtd_info *mtd_get_master(struct mtd_info *mtd)
{
	while (mtd->parent)
		mtd = mtd->parent;

	return mtd;
}

static inline u64 mtd_get_master_ofs(struct mtd_info *mtd, u64 ofs)
{
	while (mtd->parent) {
		ofs += mtd->part.offset;
		mtd = mtd->parent;
	}

	return ofs;
}

static inline uint32_t mtd_div_by_eb(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->erasesize_shift)
		return sz >> mtd->erasesize_shift;
	do_div(sz, mtd->erasesize);
	return sz;
}
#endif /* __MTD_MTD_H__ */
