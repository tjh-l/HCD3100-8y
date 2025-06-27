#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "list.h"

#define krealloc(a, b, c) realloc(a, b)
#define kfree(a) free((void *)(a))

#define kmalloc(a, b) malloc(a)
#define kcalloc(a, b, c) calloc(a, b)
#define kzalloc(a, b) zalloc(a)
#define kmalloc_array(n, size, flags) calloc(n, size)
#define BUG_ON(C) assert(!(C))
#define __user

#define max(a, b)                                                              \
	({                                                                     \
		typeof(a) _a = a;                                              \
		typeof(b) _b = b;                                              \
		_a > _b ? _a : _b;                                             \
	})

#define min(a, b)                                                              \
	({                                                                     \
		typeof(a) _a = a;                                              \
		typeof(b) _b = b;                                              \
		_a < _b ? _a : _b;                                             \
	})

#define copy_to_user(buf, data, nbytes) memcpy(buf, data, nbytes)
#define copy_from_user(data, buf, nbytes) memcpy(data, buf, nbytes)

#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

struct file {
};

struct inode {
};

struct norflash_block {
	uint32_t ofs;
	uint32_t ofs_next;
	struct list_head nodes;
	uint8_t *buffer;
	uint32_t buf_size;

	uint32_t norflash_address;
	uint32_t norflash_next_address;
	uint32_t nor_size;
};

struct nodeinfo {
	/*
	 * logical block length, for data recovery usage,
	 * we put the two bytes of len at the head of the block.
	 */
	uint16_t len;

	/*
	 * logical block start offset
	 */
	uint16_t start;
};

/* The size of "nodeinfo + crc" */
#define EXTRA_SIZE  (sizeof(struct nodeinfo) + sizeof(uint32_t))

struct bufnode {
	struct list_head node;
	struct nodeinfo info;
};

enum node_status { NODE_OK, NODE_CORRUPTED, NODE_END };

struct persistentmem_device {
	size_t size;
	const char *mtdname;
	int valid;
	void *mtd;
	size_t mtd_part_size;
	struct norflash_block blk;
};

struct vnode {
	uint16_t id;
	uint16_t size;
};

static struct persistentmem_device persistentmem_dev = { 0 };
static struct persistentmem_device *gdev = &persistentmem_dev;

static void persistentmem_lock(void)
{
}

static void persistentmem_unlock(void)
{
}

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#define __BYTE_ORDER    __LITTLE_ENDIAN

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#endif

#define tole(x) cpu_to_le32(x)
/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */

static const uint32_t crc_table[256] = {
	tole(0x00000000L), tole(0x77073096L), tole(0xee0e612cL),
	tole(0x990951baL), tole(0x076dc419L), tole(0x706af48fL),
	tole(0xe963a535L), tole(0x9e6495a3L), tole(0x0edb8832L),
	tole(0x79dcb8a4L), tole(0xe0d5e91eL), tole(0x97d2d988L),
	tole(0x09b64c2bL), tole(0x7eb17cbdL), tole(0xe7b82d07L),
	tole(0x90bf1d91L), tole(0x1db71064L), tole(0x6ab020f2L),
	tole(0xf3b97148L), tole(0x84be41deL), tole(0x1adad47dL),
	tole(0x6ddde4ebL), tole(0xf4d4b551L), tole(0x83d385c7L),
	tole(0x136c9856L), tole(0x646ba8c0L), tole(0xfd62f97aL),
	tole(0x8a65c9ecL), tole(0x14015c4fL), tole(0x63066cd9L),
	tole(0xfa0f3d63L), tole(0x8d080df5L), tole(0x3b6e20c8L),
	tole(0x4c69105eL), tole(0xd56041e4L), tole(0xa2677172L),
	tole(0x3c03e4d1L), tole(0x4b04d447L), tole(0xd20d85fdL),
	tole(0xa50ab56bL), tole(0x35b5a8faL), tole(0x42b2986cL),
	tole(0xdbbbc9d6L), tole(0xacbcf940L), tole(0x32d86ce3L),
	tole(0x45df5c75L), tole(0xdcd60dcfL), tole(0xabd13d59L),
	tole(0x26d930acL), tole(0x51de003aL), tole(0xc8d75180L),
	tole(0xbfd06116L), tole(0x21b4f4b5L), tole(0x56b3c423L),
	tole(0xcfba9599L), tole(0xb8bda50fL), tole(0x2802b89eL),
	tole(0x5f058808L), tole(0xc60cd9b2L), tole(0xb10be924L),
	tole(0x2f6f7c87L), tole(0x58684c11L), tole(0xc1611dabL),
	tole(0xb6662d3dL), tole(0x76dc4190L), tole(0x01db7106L),
	tole(0x98d220bcL), tole(0xefd5102aL), tole(0x71b18589L),
	tole(0x06b6b51fL), tole(0x9fbfe4a5L), tole(0xe8b8d433L),
	tole(0x7807c9a2L), tole(0x0f00f934L), tole(0x9609a88eL),
	tole(0xe10e9818L), tole(0x7f6a0dbbL), tole(0x086d3d2dL),
	tole(0x91646c97L), tole(0xe6635c01L), tole(0x6b6b51f4L),
	tole(0x1c6c6162L), tole(0x856530d8L), tole(0xf262004eL),
	tole(0x6c0695edL), tole(0x1b01a57bL), tole(0x8208f4c1L),
	tole(0xf50fc457L), tole(0x65b0d9c6L), tole(0x12b7e950L),
	tole(0x8bbeb8eaL), tole(0xfcb9887cL), tole(0x62dd1ddfL),
	tole(0x15da2d49L), tole(0x8cd37cf3L), tole(0xfbd44c65L),
	tole(0x4db26158L), tole(0x3ab551ceL), tole(0xa3bc0074L),
	tole(0xd4bb30e2L), tole(0x4adfa541L), tole(0x3dd895d7L),
	tole(0xa4d1c46dL), tole(0xd3d6f4fbL), tole(0x4369e96aL),
	tole(0x346ed9fcL), tole(0xad678846L), tole(0xda60b8d0L),
	tole(0x44042d73L), tole(0x33031de5L), tole(0xaa0a4c5fL),
	tole(0xdd0d7cc9L), tole(0x5005713cL), tole(0x270241aaL),
	tole(0xbe0b1010L), tole(0xc90c2086L), tole(0x5768b525L),
	tole(0x206f85b3L), tole(0xb966d409L), tole(0xce61e49fL),
	tole(0x5edef90eL), tole(0x29d9c998L), tole(0xb0d09822L),
	tole(0xc7d7a8b4L), tole(0x59b33d17L), tole(0x2eb40d81L),
	tole(0xb7bd5c3bL), tole(0xc0ba6cadL), tole(0xedb88320L),
	tole(0x9abfb3b6L), tole(0x03b6e20cL), tole(0x74b1d29aL),
	tole(0xead54739L), tole(0x9dd277afL), tole(0x04db2615L),
	tole(0x73dc1683L), tole(0xe3630b12L), tole(0x94643b84L),
	tole(0x0d6d6a3eL), tole(0x7a6a5aa8L), tole(0xe40ecf0bL),
	tole(0x9309ff9dL), tole(0x0a00ae27L), tole(0x7d079eb1L),
	tole(0xf00f9344L), tole(0x8708a3d2L), tole(0x1e01f268L),
	tole(0x6906c2feL), tole(0xf762575dL), tole(0x806567cbL),
	tole(0x196c3671L), tole(0x6e6b06e7L), tole(0xfed41b76L),
	tole(0x89d32be0L), tole(0x10da7a5aL), tole(0x67dd4accL),
	tole(0xf9b9df6fL), tole(0x8ebeeff9L), tole(0x17b7be43L),
	tole(0x60b08ed5L), tole(0xd6d6a3e8L), tole(0xa1d1937eL),
	tole(0x38d8c2c4L), tole(0x4fdff252L), tole(0xd1bb67f1L),
	tole(0xa6bc5767L), tole(0x3fb506ddL), tole(0x48b2364bL),
	tole(0xd80d2bdaL), tole(0xaf0a1b4cL), tole(0x36034af6L),
	tole(0x41047a60L), tole(0xdf60efc3L), tole(0xa867df55L),
	tole(0x316e8eefL), tole(0x4669be79L), tole(0xcb61b38cL),
	tole(0xbc66831aL), tole(0x256fd2a0L), tole(0x5268e236L),
	tole(0xcc0c7795L), tole(0xbb0b4703L), tole(0x220216b9L),
	tole(0x5505262fL), tole(0xc5ba3bbeL), tole(0xb2bd0b28L),
	tole(0x2bb45a92L), tole(0x5cb36a04L), tole(0xc2d7ffa7L),
	tole(0xb5d0cf31L), tole(0x2cd99e8bL), tole(0x5bdeae1dL),
	tole(0x9b64c2b0L), tole(0xec63f226L), tole(0x756aa39cL),
	tole(0x026d930aL), tole(0x9c0906a9L), tole(0xeb0e363fL),
	tole(0x72076785L), tole(0x05005713L), tole(0x95bf4a82L),
	tole(0xe2b87a14L), tole(0x7bb12baeL), tole(0x0cb61b38L),
	tole(0x92d28e9bL), tole(0xe5d5be0dL), tole(0x7cdcefb7L),
	tole(0x0bdbdf21L), tole(0x86d3d2d4L), tole(0xf1d4e242L),
	tole(0x68ddb3f8L), tole(0x1fda836eL), tole(0x81be16cdL),
	tole(0xf6b9265bL), tole(0x6fb077e1L), tole(0x18b74777L),
	tole(0x88085ae6L), tole(0xff0f6a70L), tole(0x66063bcaL),
	tole(0x11010b5cL), tole(0x8f659effL), tole(0xf862ae69L),
	tole(0x616bffd3L), tole(0x166ccf45L), tole(0xa00ae278L),
	tole(0xd70dd2eeL), tole(0x4e048354L), tole(0x3903b3c2L),
	tole(0xa7672661L), tole(0xd06016f7L), tole(0x4969474dL),
	tole(0x3e6e77dbL), tole(0xaed16a4aL), tole(0xd9d65adcL),
	tole(0x40df0b66L), tole(0x37d83bf0L), tole(0xa9bcae53L),
	tole(0xdebb9ec5L), tole(0x47b2cf7fL), tole(0x30b5ffe9L),
	tole(0xbdbdf21cL), tole(0xcabac28aL), tole(0x53b39330L),
	tole(0x24b4a3a6L), tole(0xbad03605L), tole(0xcdd70693L),
	tole(0x54de5729L), tole(0x23d967bfL), tole(0xb3667a2eL),
	tole(0xc4614ab8L), tole(0x5d681b02L), tole(0x2a6f2b94L),
	tole(0xb40bbe37L), tole(0xc30c8ea1L), tole(0x5a05df1bL),
	tole(0x2d02ef8dL)
};

/* ========================================================================= */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)
#else
#define DO_CRC(x) crc = tab[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
#endif

/* ========================================================================= */

/* No ones complement version. JFFS2 (and other things ?)
 * don't use ones compliment in their CRC calculations.
 */
static uint32_t __crc32_no_comp(uint32_t crc, const uint8_t *buf, uint32_t len)
{
	const uint32_t *tab = crc_table;
	const uint32_t *b = (const uint32_t *)buf;
	uint32_t rem_len;

	crc = cpu_to_le32(crc);
	/* Align it */
	if (((long)b) & 3 && len) {
		uint8_t *p = (uint8_t *)b;
		do {
			DO_CRC(*p++);
		} while ((--len) && ((long)p) & 3);
		b = (uint32_t *)p;
	}

	rem_len = len & 3;
	len = len >> 2;
	for (--b; len; --len) {
		/* load data 32 bits wide, xor data 32 bits wide. */
		crc ^= *++b; /* use pre increment for speed */
		DO_CRC(0);
		DO_CRC(0);
		DO_CRC(0);
		DO_CRC(0);
	}
	len = rem_len;
	/* And the last few bytes */
	if (len) {
		uint8_t *p = (uint8_t *)(b + 1) - 1;
		do {
			DO_CRC(*++p); /* use pre increment for speed */
		} while (--len);
	}

	return le32_to_cpu(crc);
}

static uint32_t __crc32(uint32_t crc, const uint8_t *p, uint32_t len)
{
	return __crc32_no_comp(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
}

static int flash_read_wrap(uint32_t start_addr, uint32_t read_size,
			   uint32_t *actual_size, uint8_t *buf)
{
	memcpy(buf, gdev->mtd + start_addr, read_size);
	return 0;
}

static int flash_write_wrap(uint32_t start_addr, uint32_t write_size,
			    uint32_t *actual_size, uint8_t *buf)
{
	memcpy(gdev->mtd + start_addr, buf, write_size);
	return 0;
}

static int flash_erase_wrap(unsigned long start_addr, unsigned long erase_size)
{
	memset(gdev->mtd + start_addr, 0xff, erase_size);
	return 0;
}


static int erase_block(struct norflash_block *blk)
{
	flash_erase_wrap(blk->norflash_address, blk->nor_size);
	blk->ofs = 0;
	return 0;
}

static int erase_next_block(struct norflash_block *blk)
{
	flash_erase_wrap(blk->norflash_next_address, blk->nor_size);
	blk->ofs_next = 0;
	return 0;
}

static int update_block(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	uint32_t crc;
	uint32_t actlen;
	uint8_t buf[4];
	uint32_t i;

	if (blk->ofs + 4 + (uint32_t)len > blk->nor_size) {
		printf("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	buf[i++] = ((len >> 8) & 0xff);
	buf[i++] = (len & 0xff);
	buf[i++] = ((start >> 8) & 0xff);
	buf[i++] = (start & 0xff);

	flash_write_wrap(blk->norflash_address + blk->ofs, i, &actlen, buf);

	blk->ofs += i;

	flash_write_wrap(blk->norflash_address + blk->ofs, len, &actlen,
			 buffer);

	blk->ofs += len;

	crc = __crc32(UINT32_MAX, buffer, len);
	flash_write_wrap(blk->norflash_address + blk->ofs, sizeof(crc), &actlen,
			 (uint8_t *)&crc);

	blk->ofs += sizeof(crc);

	return 0;
}

static int update_next_block(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	uint32_t crc;
	uint32_t actlen;
	uint8_t buf[4];
	uint32_t i;

	if (blk->ofs_next + 4 + len > blk->nor_size) {
		printf("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	buf[i++] = ((len >> 8) & 0xff);
	buf[i++] = (len & 0xff);
	buf[i++] = ((start >> 8) & 0xff);
	buf[i++] = (start & 0xff);

	flash_write_wrap(blk->norflash_next_address + blk->ofs_next, i, &actlen,
			 buf);

	blk->ofs_next += i;

	flash_write_wrap(blk->norflash_next_address + blk->ofs_next, len,
			 &actlen, buffer);

	blk->ofs_next += len;

	crc = __crc32(UINT32_MAX, buffer, len);
	flash_write_wrap(blk->norflash_next_address + blk->ofs_next,
			 sizeof(crc), &actlen, (uint8_t *)&crc);

	blk->ofs_next += sizeof(crc);

	return 0;
}

static int switch_block(struct norflash_block *blk)
{
	uint32_t t_ofs;
	uint32_t t_norflash_address;

	t_norflash_address = blk->norflash_address;
	blk->norflash_address = blk->norflash_next_address;
	blk->norflash_next_address = t_norflash_address;

	t_ofs = blk->ofs;
	blk->ofs = blk->ofs_next;
	blk->ofs_next = t_ofs;

	return 0;
}

static enum node_status check_node(uint8_t *pbuf)
{
	uint16_t len, start;
	uint32_t crc, crc_data;

	len = pbuf[0] << 8 | pbuf[1];

	if (len == 0xffff) {
		start = pbuf[2] << 8 | pbuf[3];
		if (start == 0xffff)
			return NODE_END; /* End of nodes. Unused free space from here. */
		else
			return NODE_CORRUPTED;
	}

	memcpy(&crc_data, pbuf + 4 + len, sizeof(crc_data));
	crc = __crc32(UINT32_MAX, pbuf + 4, len);
	if (crc != crc_data) {
		/*
		 * A bad node. Need to skip this node region.
		 */
		return NODE_CORRUPTED;
	} else {
		/*
		 * A good node.
		 */
		return NODE_OK;
	}
}

static int merge_node(struct norflash_block *blk, struct nodeinfo *pinfo)
{
	struct bufnode *pnode;
	struct list_head *pcur, *ptmp;

	pnode = kmalloc(sizeof(*pnode), GFP_KERNEL);

	memset(pnode, 0, sizeof(*pnode));

	pnode->info = *pinfo;

	if (list_empty(&blk->nodes)) {
		list_add_tail(&pnode->node, &blk->nodes);
	} else {
		list_for_each_safe (pcur, ptmp, &blk->nodes) {
			struct bufnode *p = (struct bufnode *)pcur;
			uint16_t start1 = p->info.start;
			uint16_t end1 = p->info.start + p->info.len;
			uint16_t start2 = pnode->info.start;
			uint16_t end2 = pnode->info.start + pnode->info.len;

			if (!((start1 > end2) || (end1 < start2))) {
				start2 = min(start1, start2);
				end2 = max(end1, end2);
				pnode->info.start = start2;
				pnode->info.len = end2 - start2;
				list_del(pcur);
				kfree(pcur);
			}
		}

		list_add_tail(&pnode->node, &blk->nodes);
	}

	return 0;
}

static int update_buffer(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	struct nodeinfo info;

	if (start + len > (int)blk->buf_size) {
		printf("[%s:%d]BUG: memory overflow! start 0x%08x, len %d, size %d\n",
		       __FILE__, __LINE__, start, len, blk->buf_size);
		return 0;
	}

	memcpy(blk->buffer + start, buffer, len);

	info.start = start;
	info.len = len;

	merge_node(blk, &info);

	return len;
}

static int sync_to_next_block(struct norflash_block *blk)
{
	struct list_head *pnode, *ptmp;

	if (!list_empty(&blk->nodes)) {
		list_for_each_safe (pnode, ptmp, &blk->nodes) {
			struct bufnode *p = (struct bufnode *)pnode;

			update_next_block(blk, p->info.start, p->info.len,
					  blk->buffer + p->info.start);
		}
	}

	return 0;
}

static enum node_status scan_block(struct norflash_block *blk)
{
	uint8_t *pnode_head;
	uint16_t len, start;
	enum node_status ret = NODE_END;
	uint8_t *buf;
	uint32_t actlen;

	buf = kmalloc(blk->nor_size, GFP_KERNEL);
	BUG_ON(buf == NULL);

	flash_read_wrap(blk->norflash_address, blk->nor_size, &actlen, buf);
	pnode_head = buf;
	blk->ofs = 0;

	for (;;) {
		switch (check_node(pnode_head)) {
		case NODE_END:
			/*
			 * norflash scan over.
			 */
			ret = NODE_END;
			goto scan_over;

		case NODE_OK:
			len = pnode_head[0] << 8 | pnode_head[1];
			start = pnode_head[2] << 8 | pnode_head[3];

			update_buffer(blk, start, len, pnode_head + 4);

			break;

		case NODE_CORRUPTED:
			printf("Found corrupted node\r\n");
			ret = NODE_CORRUPTED;
			goto scan_over;

		default:
			break;
		}

		if (blk->ofs + len + EXTRA_SIZE >= blk->nor_size) {
			/*
			 * norflash scan over.
			 */
			ret = NODE_END;
			goto scan_over;
		} else {
			blk->ofs += len + EXTRA_SIZE;
			pnode_head += len + EXTRA_SIZE;
		}
	}

scan_over:
	kfree(buf);

	return ret;
}

#if 0
static void reset_block_and_nodes(void)
{
	struct list_head *pnode, *ptmp;

	if (gdev->blk.buffer) {
		memset(gdev->blk.buffer, 0, gdev->blk.buf_size);
	}

	if (!list_empty(&gdev->blk.nodes)) {
		list_for_each_safe(pnode, ptmp, &gdev->blk.nodes) {
			list_del(pnode);
			kfree(pnode);
		}
	}
}
#endif

static int prepare_block(struct norflash_block *blk)
{
	uint8_t *buf;
	uint32_t crc, crc_data;
	uint32_t ncrc, ncrc_data;
	bool blk_is_new, nblk_is_new;
	uint32_t i;
	uint32_t actlen;

	buf = kmalloc(blk->nor_size*2, GFP_KERNEL);
	BUG_ON(buf == NULL);

	flash_read_wrap(blk->norflash_address, blk->nor_size*2, &actlen, buf);

	memcpy(&crc_data, buf + blk->nor_size - 4, sizeof(crc_data));
	crc = __crc32(UINT32_MAX, buf, blk->nor_size - 4);

	memcpy(&ncrc_data, buf + blk->nor_size * 2 - 4, sizeof(crc_data));
	ncrc = __crc32(UINT32_MAX, buf + blk->nor_size, blk->nor_size - 4);

	BUG_ON(crc == crc_data && ncrc == ncrc_data);

	blk_is_new = true;
	nblk_is_new = true;

	for (i = 0; i < blk->nor_size; i++) {
		if ((*(buf + i)) != 0xff) {
			blk_is_new = false;
			break;
		}
	}

	for (i = 0; i < blk->nor_size; i++) {
		if ((*(buf + blk->nor_size + i)) != 0xff) {
			nblk_is_new = false;
			break;
		}
	}

	if (blk_is_new && nblk_is_new)
		return -1;

	if (blk_is_new) {
		/*
		 * norflash is new erased block.
		 * So, use norflash_next.
		 */
		printf("Now, use norflash_next!\n");

		switch_block(blk);

		if (scan_block(blk) == NODE_CORRUPTED) {
			printf("Block is corrupted\r\n");
			erase_next_block(blk);
			sync_to_next_block(blk);
			erase_block(blk);
			switch_block(blk);
			//reset_block_and_nodes();
			//erase_block(blk);
			//assert(scan_block(blk) == NODE_END);
		}
		kfree(buf);

		return 0;
	}

	if (nblk_is_new) {
		/*
		 * norflash_next is new erased block.
		 * So, use norflash.
		 */
		printf("Now, use norflash!\n");

		if (scan_block(blk) == NODE_CORRUPTED) {
			printf("Block is corrupted\r\n");
			erase_next_block(blk);
			sync_to_next_block(blk);
			erase_block(blk);
			switch_block(blk);
			//reset_block_and_nodes();
			//erase_block(blk);
			//assert(scan_block(blk) == NODE_END);
		}
		kfree(buf);

		return 0;
	}

	if (crc == crc_data) {
		/*
		 * norflash write complete, but switch to
		 * norflash_next not finished.
		 * Now, finished the switch again.
		 */
		printf("norflash write complete, but switch to\n"
			 "norflash_next not finished.\n"
			 "Now, finished the switch again.\n");

		scan_block(blk);
		erase_next_block(blk);
		sync_to_next_block(blk);
		erase_block(blk);
		switch_block(blk);
		kfree(buf);

		return 0;
	}

	if (ncrc == ncrc_data) {
		/*
		 * norflash_next write complete, but switch to
		 * norflash not finished.
		 * Now, finished the switch again.
		 */
		printf("norflash_next write complete, but switch to\n"
			 "norflash not finished.\n"
			 "Now, finished the switch again.\n");

		switch_block(blk);
		scan_block(blk);
		erase_next_block(blk);
		sync_to_next_block(blk);
		erase_block(blk);
		switch_block(blk);
		kfree(buf);

		return 0;
	}

	if (crc_data != 0xFFFFFFFF && ncrc_data == 0xFFFFFFFF) {
		printf("norflash_next write complete, but erase not finished.\n"
		      "Now, using norflash_next and erase norflash.\n");

		switch_block(blk);
		scan_block(blk);
		erase_next_block(blk);
		kfree(buf);

		return 0;
	}

	if (crc_data == 0xFFFFFFFF && ncrc_data != 0xFFFFFFFF) {
		printf("norflash write complete, but erase not finished.\n"
		      "Now, using norflash and erase norflash_next.\n");

		scan_block(blk);
		erase_next_block(blk);
		kfree(buf);

		return 0;
	}

	/*
	 * BUG: Because the erase operation may not be completely finished.
	 * both norflash and norflash_next may be broken!!!
	 * Need to check which one need to complete the erase operation.
	 */
	printf("BUG: Because the erase operation may not be completely finished.\n");
	printf("both norflash and norflash_next may be broken!!!\n");
	printf("Need to check which one need to complete the erase operation.\n");

	blk_is_new = true;
	nblk_is_new = true;

	for (i = 0; i < 4; i++) {
		if ((*(buf + i)) != 0xff) {
			blk_is_new = false;
			break;
		}
	}

	for (i = 0; i < 4; i++) {
		if ((*(buf + blk->nor_size + i)) != 0xff) {
			nblk_is_new = false;
			break;
		}
	}

	if (!blk_is_new && nblk_is_new) {
		BUG_ON(scan_block(blk) != NODE_END);
		erase_next_block(blk);
	} else if (blk_is_new && !nblk_is_new) {
		switch_block(blk);
		BUG_ON(scan_block(blk) != NODE_END);
		erase_next_block(blk);
	} else {
		printf("Persistentmem is corrupted\r\n");
		while(1);
		erase_block(blk);
		erase_next_block(blk);
	}

	kfree(buf);

	return 0;
}

static int free_size(struct norflash_block *blk)
{
	if ((blk->ofs + EXTRA_SIZE + sizeof(uint32_t)) < blk->nor_size)
		return (blk->nor_size - EXTRA_SIZE - blk->ofs - sizeof(uint32_t));
	else
		return 0;
}

static uint32_t calc_crc32(struct norflash_block *blk)
{
	uint8_t buf[512];
	uint32_t crc = UINT32_MAX;
	uint32_t actlen;
	uint32_t read_size = 0;
	uint32_t piece;
	uint8_t last_piece = 0;

	while (read_size < blk->nor_size) {
		if (blk->nor_size - read_size > 512) {
			piece = 512;
		} else {
			piece = blk->nor_size - read_size;
			last_piece = 1;
		}
		flash_read_wrap(blk->norflash_address + read_size, piece, &actlen, buf);
		if (last_piece)
			crc = __crc32(crc, buf, piece - 4);
		else
			crc = __crc32(crc, buf, piece);

		read_size += piece;
	}

	return crc;
}


static int wear_level_memory_write(struct norflash_block *blk, int start,
				   int len, uint8_t *buffer)
{
	uint32_t crc;
	uint32_t actlen;

	if (len == 0)
		return 0;

	if (len > free_size(blk)) {
		/*
		 * save crc of whole block.
		 */
		crc = calc_crc32(blk);

		flash_write_wrap(blk->norflash_address + blk->nor_size - 4,
				 sizeof(crc), &actlen, (uint8_t *)&crc);

		/*
		 * update buffer to next block.
		 */
		update_buffer(blk, start, len, buffer);
		sync_to_next_block(blk);

		/*
		 * erase current block.
		 */
		erase_block(blk);

		/*
		 * switch to next block
		 */
		switch_block(blk);

		return len;
	}

	/* Update write operation to NorFlash */
	update_block(blk, start, len, buffer);

	/* Update write operation to temperory norflash buffer */
	update_buffer(blk, start, len, buffer);

	return len;
}

static int wear_level_memory_read(struct norflash_block *blk,
		int start, int len, uint8_t *buffer)
{
	if (len == 0)
		return 0;

	if (start + len > (int)blk->buf_size) {
		printf("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	memcpy(buffer, blk->buffer + start, len);

	return len;
}

static void flash_wear_leveling_exit(void)
{
	struct list_head *pnode, *ptmp;

	if (gdev->blk.buffer) {
		kfree(gdev->blk.buffer); 
		gdev->blk.buffer = NULL;
	}

	if (!list_empty(&gdev->blk.nodes)) {
		list_for_each_safe(pnode, ptmp, &gdev->blk.nodes) {
			list_del(pnode);
			kfree(pnode);
		}
	}
}

static int flash_wear_leveling_init(void)
{
	uint8_t *buffer = NULL;
	int ret = 0;

	buffer = (uint8_t *)kmalloc(gdev->size, GFP_KERNEL);
	if (!buffer) {
		printf("malloc failed!\r\n");
		ret = -ENOMEM;
		goto error;
	}
	memset(buffer, 0x0, gdev->size);

	gdev->blk.buffer = buffer;
	gdev->blk.buf_size = gdev->size;
	gdev->blk.norflash_address = 0;
	gdev->blk.norflash_next_address = gdev->mtd_part_size >> 1;
	gdev->blk.nor_size = gdev->mtd_part_size >> 1;
	INIT_LIST_HEAD(&gdev->blk.nodes);

	ret = prepare_block(&gdev->blk);
	if (ret) {
		printf("No data to read\r\n");
	}

	printf("Flash init success!\r\n");
	return 0;

error:
	if (buffer) {
		kfree(buffer);
		gdev->blk.buffer = NULL;
		buffer = NULL;	
	}

	return ret;
}

static bool __persistentmem_read(const uint32_t addr, uint8_t *const data,
				 const uint32_t data_len)
{
	int len;

	len = wear_level_memory_read(&gdev->blk, addr, data_len, data);

	if (len != data_len) {
		return false;
	}

	return true;
}

static bool __persistentmem_write(const uint32_t addr,
				  const uint8_t *const data,
				  const uint32_t data_len)
{
	int len;

	len = wear_level_memory_write(&gdev->blk, addr, data_len, (uint8_t *)data);

	if (len != data_len) {
		printf("Write error, len:%d, data_len:%d\r\n", len, data_len);
		return false;
	}

	return true;
}

static int persistentmem_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int persistentmem_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t persistentmem_read(struct file *file, char __user *buf,
				  size_t nbytes, loff_t *ppos)
{

	uint32_t addr = (*ppos);
	uint8_t *data = NULL;

	persistentmem_lock();

	if (gdev->valid) {
		data = kmalloc(nbytes, GFP_KERNEL);
		if (!data) {
			persistentmem_unlock();
			return -ENOMEM;
		}

		if (!__persistentmem_read(addr, data, nbytes)) {
			kfree(data);
			persistentmem_unlock();
			return -EFAULT;
		}

		if (copy_to_user(buf, data, nbytes)) {
			kfree(data);
			persistentmem_unlock();
			return -EFAULT;
		}

		kfree(data);

		*ppos = addr + nbytes;
	} else {
		nbytes = 0;
	}

	persistentmem_unlock();

	return nbytes;
}

static ssize_t persistentmem_write(struct file *file, const char __user *buf,
				   size_t nbytes, loff_t *ppos)
{
	uint32_t addr = (*ppos);
	uint8_t *data = NULL;

	persistentmem_lock();
	if (gdev->valid) {
		data = kmalloc(nbytes, GFP_KERNEL);
		if (!data) {
			persistentmem_unlock();
			return -ENOMEM;
		}

		if (copy_from_user(data, buf, nbytes)) {
			kfree(data);
			persistentmem_unlock();
			return -EFAULT;
		}

		if (!__persistentmem_write(addr, data, nbytes)) {
			kfree(data);
			persistentmem_unlock();
			return -EFAULT;
		}

		kfree(data);

		*ppos = addr + nbytes;
	} else {
		nbytes = 0;
	}

	persistentmem_unlock();
	return nbytes;
}

static void *node_find(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *p = buffer;

	while (p < buffer + size) {
		pvnode = (struct vnode *)p;
		if (pvnode->id == id)
			return p;
		if (pvnode->id == PERSISTENTMEM_NODE_ID_UNUSED)
			return NULL;

		p += (sizeof(*pvnode) + pvnode->size);
	}

	return NULL;
}

static int node_create(void *buffer, uint32_t size, struct persistentmem_node_create *node)
{
	struct vnode vnode;
	void *p = buffer;
	int len;

	/* check if id duplicate */
	if (node_find(buffer, size, node->id)) {
		return PERSISTENTMEM_ERR_ID_DUPLICATED;
	}

	p = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	if ((p + sizeof(struct vnode) + node->size) >= (buffer + size)) {
		return PERSISTENTMEM_ERR_NO_SPACE;
	}

	vnode.id = node->id;
	vnode.size = node->size;
	len = wear_level_memory_write(&gdev->blk, (int)(p - buffer),
				      sizeof(struct vnode), (uint8_t *)&vnode);
	if (len != sizeof(struct vnode)) {
		printf("Write error, len:%d, data_len:%d\r\n", len, sizeof(struct vnode));
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static int node_delete(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *pcur = NULL;
	void *pnext = NULL;
	void *pend = NULL;
	int len;
	void *tmp;

	pcur = node_find(buffer, size, id);
	if (!pcur) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	pend = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	pvnode = (struct vnode *)pcur;
	pnext = pcur + sizeof(struct vnode) + pvnode->size;
	tmp = kmalloc(pend - pcur, GFP_KERNEL);
	if (!tmp) {
		return -ENOMEM;
	}

	if (pnext == pend) {
		memset(tmp, 0, pend - pcur);
	} else {
		memcpy(tmp, pnext, pend - pnext);
		memset(tmp + (pend - pnext), 0, (pnext - pcur));
	}

	len = wear_level_memory_write(&gdev->blk, (int)(pcur - buffer),
				      (int)(pend - pcur), (uint8_t *)tmp);
	if (len != (pend - pcur)) {
		printf("Write error, len:%d, data_len:%d\r\n", len, pend - pcur);
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static int node_size(int id)
{
	struct vnode *pvnode;

	pvnode = node_find(gdev->blk.buffer, gdev->blk.buf_size, id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	return pvnode->size;
}

static int node_get(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	memcpy(node->buf, (char *)pvnode + sizeof(struct vnode) + node->offset, node->size);

	return 0;
}

static int node_put(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;
	int start, len;

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	start = (int)((void *)pvnode + sizeof(struct vnode) + node->offset - buffer);
	len = wear_level_memory_write(&gdev->blk, start, node->size, (uint8_t *)node->buf);
	if (len != node->size) {
		printf("Write error, len:%d, data_len:%d\r\n", len, node->size);
		return PERSISTENTMEM_ERR_FAULT;
	}

	return 0;
}

static long __persistentmem_ioctl(struct file *file, unsigned int cmd, unsigned long parg, int is_kernel)
{
	int rc = 0;

	switch (cmd) {
	case PERSISTENTMEM_IOCTL_NODE_CREATE: {
		persistentmem_lock();
		if (gdev->valid) {
			struct persistentmem_node_create node;
			if (!is_kernel)
				copy_from_user((void *)&node, (void *)parg, sizeof(node));
			else
				memcpy((void *)&node, (void *)parg, sizeof(node));
			node.size = ((node.size + 3) >> 2) << 2;
			rc = node_create(gdev->blk.buffer, gdev->blk.buf_size, &node);
		} else {
			rc = -1;
		}
		persistentmem_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_DELETE: {
		persistentmem_lock();
		if (gdev->valid) {
			rc = node_delete(gdev->blk.buffer, gdev->blk.buf_size, parg);
		} else {
			rc = -1;
		}
		persistentmem_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_GET: {
		persistentmem_lock();
		if (gdev->valid) {
			struct persistentmem_node node;
			struct persistentmem_node node_user;
			void *buf;
			if (!is_kernel)
				copy_from_user((void *)&node_user, (void *)parg, sizeof(node_user));
			else
				memcpy((void *)&node_user, (void *)parg, sizeof(node_user));
			memcpy(&node, &node_user, sizeof(node_user));
			buf = kmalloc(node.size, GFP_KERNEL);
			if (!buf)
				return -ENOMEM;
			node.buf = buf;
			rc = node_get(gdev->blk.buffer, gdev->blk.buf_size, &node);
			if (!rc) {
				if (!is_kernel)
					copy_to_user(node_user.buf, buf, node_user.size);
				else
					memcpy(node_user.buf, buf, node_user.size);
			}
			kfree(buf);
		} else
			rc = -1;
		persistentmem_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_PUT: {
		persistentmem_lock();
		if (gdev->valid) {
			struct persistentmem_node node;
			void *buf;
			if (!is_kernel)
				copy_from_user((void *)&node, (void *)parg, sizeof(node));
			else
				memcpy((void *)&node, (void *)parg, sizeof(node));
			buf = kmalloc(node.size, GFP_KERNEL);
			if (!buf)
				return -ENOMEM;
			if (!is_kernel)
				copy_from_user(buf, node.buf, node.size);
			else
				memcpy(buf, node.buf, node.size);
			node.buf = buf;
			rc = node_put(gdev->blk.buffer, gdev->blk.buf_size, &node);
			kfree(buf);
		} else
			rc = -1;
		persistentmem_unlock();
		break;
	}
	case PERSISTENTMEM_IOCTL_MARK_INVALID: {
		persistentmem_lock();
		gdev->valid = 0;
		persistentmem_unlock();
		break;
	}
	default:
		break;
	}

	return rc;
}

static long persistentmem_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
	return __persistentmem_ioctl(file, cmd, parg, 0);
}

long persistentmem_ioctl_internal(unsigned int cmd, unsigned long parg)
{
	return __persistentmem_ioctl(NULL, cmd, parg, 1);
}

static void dump_sysdata(struct sysdata *sysdata)
{
	printf("product_id:\r\n");
	printf("\t%s\r\n", sysdata->product_id);

	printf("firmware_version:\r\n");
	printf("\t%ld\r\n", sysdata->firmware_version);

	printf("ota_detect_modes:\r\n");
	printf("\t%ld\r\n", sysdata->ota_detect_modes);

	printf("tvtype:\r\n");
	printf("\t%d\r\n", sysdata->tvtype);

	printf("volume:\r\n");
	printf("\t%d\r\n", sysdata->volume);

	printf("flip_mode:\r\n");
	printf("\t%d\r\n", sysdata->flip_mode);

	printf("lcd_pwm_backlight:\r\n");
	printf("\t%d\r\n", sysdata->lcd_pwm_backlight);

	printf("lcd_pwm_vcom:\r\n");
	printf("\t%d\r\n", sysdata->lcd_pwm_vcom);

	printf("adc_adjust_value:\r\n");
	printf("\t%d\r\n", sysdata->adc_adjust_value);

	printf("ota_doing:\r\n");
	printf("\t%d\r\n", sysdata->ota_doing);

	printf("disp_rect_x:\r\n");
	printf("\t%d\r\n", sysdata->disp_rect_x);

	printf("disp_rect_y:\r\n");
	printf("\t%d\r\n", sysdata->disp_rect_y);

	printf("disp_rect_w:\r\n");
	printf("\t%d\r\n", sysdata->disp_rect_w);

	printf("disp_rect_h:\r\n");
	printf("\t%d\r\n", sysdata->disp_rect_h);
}

static void hex_dump(const char *desc, const void *addr, size_t len)
{
	uint8_t *p = (uint8_t *)addr;
	uint8_t b[16];
	size_t i, j;

	// 打印描述信息
	if (desc != NULL) {
		printf("%s:\n", desc);
	}

	// 遍历内存块
	for (i = 0; i < len; i += 16) {
		// 打印每行的十六进制数据
		for (j = 0; j < sizeof(b); j++) {
			if (i + j < len) {
				printf("%02x ", (uint8_t)p[i + j]);
			} else {
				printf("   "); // 填充空位
			}
		}

		// 打印空格
		printf("  ");

		// 打印ASCII文本表示
		printf("  ");
		for (j = 0; j < sizeof(b); j++) {
			if (i + j < len) {
				b[j] = p[i + j];
				if (b[j] >= 32 &&
				    b[j] <= 126) { // 可打印ASCII字符
					printf("%c", b[j]);
				} else {
					printf("."); // 非打印字符用点表示
				}
			}
		}
		printf("\n");
	}
}

static void dump_node(struct persistentmem_node *node)
{
	unsigned char *data = node->buf;
	int i, j;

	printf("node id:   %d\r\n", node->id);
	printf("node size: %d\r\n", node->size);
	hex_dump("vnode", node->buf, node->size);
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-iolsd]\n", prog);
	puts("  -i --input        input file\n"
	     "  -o --offset       persistentmem offset in file\n"
	     "  -l --length       persistentmem partition length\n"
	     "  -s --size         persistentmem simulate memory size\n"
	     "  -d --dump         dump contents of NODE\n");
}

int main(int argc, char *argv[])
{
	const char *ifile = NULL;
	int mtd_offset = 0;
	int mtd_size = 0;
	int simulate_size = 0;
	int dump_nodes[128];
	int num_dump = 0;
	FILE *fp;
	int ret;

	opterr = 0;
	optind = 0;
	while (1) {
		static const struct option lopts[] = {
			{ "input", 1, 0, 'i' },
			{ "offset", 1, 0, 'o' },
			{ "length", 1, 0, 'l' },
			{ "size", 1, 0, 's' },
			{ "dump", 1, 0, 'd' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "i:o:l:s:d:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'i':
			ifile = optarg;
			break;
		case 'o':
			mtd_offset = (int)strtoll(optarg, NULL, 0);
			break;
		case 'l':
			mtd_size = (int)strtoll(optarg, NULL, 0);
			break;
		case 's':
			simulate_size = (int)strtoll(optarg, NULL, 0);
			break;
		case 'd':
			dump_nodes[num_dump++] = (int)strtoll(optarg, NULL, 0);
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!ifile) {
		printf("Please specify input file\r\n");
		print_usage(argv[0]);
		return -1;
	}

	if (simulate_size == 0) {
		printf("Please specify size\r\n");
		print_usage(argv[0]);
		return -1;
	}

	if (mtd_size == 0) {
		printf("Please specify length\r\n");
		print_usage(argv[0]);
		return -1;
	}

	fp = fopen(ifile, "rb");
	if (!fp) {
		printf("open %s failed\r\n", ifile);
		return -1;
	}

	gdev->mtd = calloc(1, mtd_size);
	gdev->size = simulate_size;
	gdev->mtd_part_size = mtd_size;
	gdev->valid = 1;

	fseek(fp, mtd_offset, SEEK_SET);
	fread(gdev->mtd, 1, mtd_size, fp);
	ret = flash_wear_leveling_init();
	if (ret) {
		printf("persistentmem initialize failed\r\n");
		return -1;
	}

	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (persistentmem_ioctl_internal(PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		printf("get syddata failed\r\n");
	} else {
		dump_sysdata(&sysdata);
	}

	for (int i = 0; i < num_dump; i++) {
		int nid = dump_nodes[i];
		int nsize = node_size(nid);
		printf("dump NODE %d:\r\n", nid);
		if (nsize < 0) {
			printf("Not found vnode %d\r\n", nid);
			continue;
		}

		void *buf = calloc(1, nsize);
		memset(&node, 0, sizeof(node));
		node.id = nid;
		node.offset = 0;
		node.size = nsize;
		node.buf = buf;
		if (persistentmem_ioctl_internal(PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
			printf("get node %d failed\r\n", nid);
		} else {
			dump_node(&node);
		}

		free(buf);
	}

	fclose(fp);
	return 0;
}
