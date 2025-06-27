#ifndef _LINUX_VMALLOC_H
#define _LINUX_VMALLOC_H

#define vmalloc(s) malloc(s)
#define vzalloc(s) kzalloc(s, a)
#define vfree(p) free(p)

#endif /* _LINUX_VMALLOC_H */
