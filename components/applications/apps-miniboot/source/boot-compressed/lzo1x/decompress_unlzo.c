#include "lzo1x.h"

int __decompress(unsigned char *buf, long in_len,
			      long (*fill)(void*, unsigned long),
			      long (*flush)(void*, unsigned long),
			      unsigned char *output, long out_len,
			      long *posp, unsigned long *pdecomp_len,
			      void (*error)(char *x))
{
	lzo1x_decompress((const unsigned char *)buf, in_len, (unsigned char *)output, pdecomp_len, NULL);
	return 0;
}
