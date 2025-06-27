/*
 * Copyright 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * OF helpers for mtd.
 *
 * This file is released under the GPLv2
 */

#ifndef __LINUX_OF_MTD_H
#define __LINUX_OF_MTD_H

#include <stdbool.h>
#include <linux/of.h>

int of_get_nand_ecc_mode(int np);
int of_get_nand_ecc_step_size(int np);
int of_get_nand_ecc_strength(int np);
int of_get_nand_bus_width(int np);
bool of_get_nand_on_flash_bbt(int np);

#endif /* __LINUX_OF_MTD_H */
