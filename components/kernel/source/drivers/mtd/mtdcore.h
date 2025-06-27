/* SPDX-License-Identifier: GPL-2.0 */
/*
 * These are exported solely for the purpose of mtd_blkdevs.c and mtdchar.c.
 * You should not use them for _anything_ else.
 */

extern struct mutex mtd_table_mutex;
extern struct backing_dev_info *mtd_bdi;

struct mtd_info *__mtd_next_device(int i);
int __must_check add_mtd_device(struct mtd_info *mtd);
int __init mtdchar_register(const char *devpath, struct mtd_info *priv);

struct mtd_partitions;

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))
