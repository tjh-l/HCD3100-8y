#ifndef _ASM_UACCESS_H
#define _ASM_UACCESS_H

/*
 * USER_DS is a bitmask that has the bits set that may not be set in a valid
 * userspace address.  Note that we limit 32-bit userspace to 0x7fff8000 but
 * the arithmetic we're doing only works if the limit is a power of two, so
 * we use 0x80000000 here on 32-bit kernels.  If a process passes an invalid
 * address in this range it's the process's problem, not ours :-)
 */

typedef unsigned long mm_segment_t;

#ifdef CONFIG_KVM_GUEST
#define KERNEL_DS       ((mm_segment_t) { 0x80000000UL })
#define USER_DS         ((mm_segment_t) { 0xC0000000UL })
#else
#define KERNEL_DS       ((mm_segment_t) { 0UL })
#define USER_DS         ((mm_segment_t) { __UA_LIMIT })
#endif

#define VERIFY_READ    0
#define VERIFY_WRITE   1

#define access_ok(type, addr, size) 1
#define copy_to_user(dest, src, len)                                           \
	({                                                                     \
		long __cu_len;                                                 \
		__cu_len = (0);                                                \
		memcpy(dest, src, len);                                        \
		__cu_len;                                                      \
	})

#define copy_from_user(dest, src, len)                                         \
	({                                                                     \
		long __cu_len;                                                 \
		__cu_len = (0);                                                \
		memcpy(dest, src, len);                                        \
		__cu_len;                                                      \
	})

#endif /* _ASM_UACCESS_H */
