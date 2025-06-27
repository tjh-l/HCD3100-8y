int lzmaBuffToBuffDecompress(unsigned char *outData, unsigned long *deompressedSize,
        unsigned char *inData, unsigned long inLength);

int __decompress(unsigned char *buf, long in_len,
			      long (*fill)(void*, unsigned long),
			      long (*flush)(void*, unsigned long),
			      unsigned char *output, long out_len,
			      long *posp, unsigned long *pdecomp_len,
			      void (*error)(char *x))
{
	unsigned long s = 0x10000000UL;
	lzmaBuffToBuffDecompress((unsigned char *)output, &s,
				 (unsigned char *)buf, in_len);
	if (s != 0x10000000UL && pdecomp_len)
		*pdecomp_len = s;
	return 0;
}
