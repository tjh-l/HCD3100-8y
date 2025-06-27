#ifndef __HCCAST_DSC_H__
#define __HCCAST_DSC_H__

void* hccast_dsc_aes_ctr_open(int mmap_size);
void* hccast_dsc_aes_cbc_open(int mmap_size);
void hccast_dsc_ctx_destroy(void* ctx);
int hccast_dsc_decrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len);
int hccast_dsc_encrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len);

#endif
