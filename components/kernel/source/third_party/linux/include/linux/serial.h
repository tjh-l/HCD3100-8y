/*
 * include/linux/serial.h
 *
 * Copyright (C) 1992 by Theodore Ts'o.
 * 
 * Redistribution of this file is permitted under the terms of the GNU 
 * Public License (GPL)
 */
#ifndef _LINUX_SERIAL_H
#define _LINUX_SERIAL_H

//#include <asm/page.h>
//#include <uapi/linux/serial.h>


/*
 * Counters of the input lines (CTS, DSR, RI, CD) interrupts
 */

struct async_icount {
	__u32	cts, dsr, rng, dcd, tx, rx;
	__u32	frame, parity, overrun, brk;
	__u32	buf_overrun;
};

/* modem lines */
#define TIOCM_LE	0x001		/* line enable */
#define TIOCM_DTR	0x002		/* data terminal ready */
#define TIOCM_RTS	0x004		/* request to send */
#define TIOCM_ST	0x010		/* secondary transmit */
#define TIOCM_SR	0x020		/* secondary receive */
#define TIOCM_CTS	0x040		/* clear to send */
#define TIOCM_CAR	0x100		/* carrier detect */
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RNG	0x200		/* ring */
#define TIOCM_RI	TIOCM_RNG
#define TIOCM_DSR	0x400		/* data set ready */
#define TIOCM_OUT1	0x2000
#define TIOCM_OUT2	0x4000
#define TIOCM_LOOP	0x8000

/* I hope the range from 0x5480 on is free ... */
#define TIOCSCTTY	0x5480		/* become controlling tty */
#define TIOCGSOFTCAR	0x5481
#define TIOCSSOFTCAR	0x5482
#define TIOCLINUX	0x5483
#define TIOCGSERIAL	0x5484
#define TIOCSSERIAL	0x5485
#define TCSBRKP		0x5486	/* Needed for POSIX tcsendbreak() */
#define TIOCSERCONFIG	0x5488
#define TIOCSERGWILD	0x5489
#define TIOCSERSWILD	0x548a
#define TIOCGLCKTRMIOS	0x548b
#define TIOCSLCKTRMIOS	0x548c
#define TIOCSERGSTRUCT	0x548d /* For debugging only */
#define TIOCSERGETLSR	0x548e /* Get line status register */
#define TIOCSERGETMULTI 0x548f /* Get multiport config	*/
#define TIOCSERSETMULTI 0x5490 /* Set multiport config */
#define TIOCMIWAIT	0x5491 /* wait for a change on serial input line(s) */
#define TIOCGICOUNT	0x5492 /* read serial port inline interrupt counts */


struct serial_struct {
	int	type;
	int	line;
	unsigned int	port;
	int	irq;
	int	flags;
	int	xmit_fifo_size;
	int	custom_divisor;
	int	baud_base;
	unsigned short	close_delay;
	char	io_type;
	char	reserved_char[1];
	int	hub6;
	unsigned short	closing_wait; /* time to wait before closing */
	unsigned short	closing_wait2; /* no longer used... */
	unsigned char	*iomem_base;
	unsigned short	iomem_reg_shift;
	unsigned int	port_high;
	unsigned long	iomap_base;	/* cookie passed into ioremap */
};

/*
 * For the close wait times, 0 means wait forever for serial port to
 * flush its output.  65535 means don't wait at all.
 */
#define ASYNC_CLOSING_WAIT_INF	0
#define ASYNC_CLOSING_WAIT_NONE	65535


/*
 * The size of the serial xmit buffer is 1 page, or 4096 bytes
 */
#define SERIAL_XMIT_SIZE PAGE_SIZE

#include <linux/compiler.h>

#endif /* _LINUX_SERIAL_H */
