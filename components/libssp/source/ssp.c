/*
 * Copyright (C) 2019 X-Sail Technology Co,. LTD.
 *
 * Authors:  X-Sail
 *
 * This source is confidential and is  X-Sail's proprietary information.
 * This source is subject to  X-Sail License Agreement, and shall not be
 * disclosed to unauthorized individual.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

#define SR_IE                  0x00000001      /* Interrupt Enable, current */

#define __write_32bit_c0_register(register, sel, value)			\
do {									\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mtc0\t%z0, " #register "\n\t"			\
			: : "Jr" ((unsigned int)(value)));		\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mtc0\t%z0, " #register ", " #sel "\n\t"	\
			".set\tmips0"					\
			: : "Jr" ((unsigned int)(value)));		\
} while (0)


#define __read_32bit_c0_register(source, sel)				\
({ unsigned int __res;							\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mfc0\t%0, " #source "\n\t"			\
			: "=r" (__res));				\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mfc0\t%0, " #source ", " #sel "\n\t"		\
			".set\tmips0\n\t"				\
			: "=r" (__res));				\
	__res;								\
})

#define read_c0_status() __read_32bit_c0_register($12, 0)
#define write_c0_status(val) __write_32bit_c0_register($12, 0, val)
//#define LIBSSP_DATA __attribute__ ((section(".stack_chk_guard")))

void *__stack_chk_guard;
//LIBSSP_DATA void *__stack_chk_guard;
void __stack_chk_fail(void)
{
	unsigned int reg_val;

	/* disable all interrupt */
	reg_val = read_c0_status();
	reg_val &= ~(SR_IE);
	write_c0_status(reg_val);

	while (1)
		;
}
