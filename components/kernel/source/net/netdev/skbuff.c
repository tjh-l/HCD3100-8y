#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/bug.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/mm.h>
#include <errno.h>

static void skb_panic(struct sk_buff *skb, unsigned int sz, void *addr,
		      const char msg[])
{
	pr_emerg(
		"%s: text:%p len:%d put:%d head:%p data:%p tail:%#lx end:%#lx dev:%s\n",
		msg, addr, skb->len, sz, skb->head, skb->data,
		(unsigned long)skb->tail, (unsigned long)skb->end,
		"<NULL>");
	BUG();
}

static void skb_over_panic(struct sk_buff *skb, unsigned int sz, void *addr)
{
	skb_panic(skb, sz, addr, __func__);
}

static void skb_under_panic(struct sk_buff *skb, unsigned int sz, void *addr)
{
	skb_panic(skb, sz, addr, __func__);
}

void __kfree_skb(struct sk_buff *skb)
{
	if (skb) {
		if (skb->flags & SKBUF_FLAG_IS_HCLONE) {
			struct sk_buff_hclones *hclones = (struct sk_buff_hclones *)skb;
			hclones->pskb2->hclone_free_function(hclones->pskb2);
		}

		if (skb->flags & SKBUF_FLAG_IS_CUSTOM) {
			skb->custom_free_function(skb);
		} else {
			kfree(skb);
		}
	}
}

void consume_skb(struct sk_buff *skb)
{
	__kfree_skb(skb);
}

void dev_kfree_skb_any(struct sk_buff *skb)
{
	dev_kfree_skb(skb);
}

struct sk_buff *alloc_skb(unsigned int size, gfp_t priority)
{
#ifdef CONFIG_NET_SKB_HEAD_RESERVE
	size_t alloc_size = sizeof(struct sk_buff) + size + 3 * ARCH_DMA_MINALIGN;
#else
	size_t alloc_size = sizeof(struct sk_buff) + size + 2 * ARCH_DMA_MINALIGN;
#endif
	struct sk_buff *skb = (void *)malloc(alloc_size);

	if (skb == NULL) {
		return NULL;
	}

	memset(skb, 0, sizeof(struct sk_buff));
	skb->head = skb->data = (unsigned char *)skb + ALIGN(sizeof(struct sk_buff), ARCH_DMA_MINALIGN);
	skb_reset_tail_pointer(skb);
	skb->end = (uint8_t *)skb + alloc_size;
	skb->truesize = skb->end - skb->head;

#ifdef CONFIG_NET_SKB_HEAD_RESERVE
	/* reserve ARCH_DMA_MINALIGN to save head in monitor case */
	skb_reserve(skb, ARCH_DMA_MINALIGN * 2) ;
#endif

	return skb;
}

struct sk_buff *__netdev_alloc_skb(struct net_device *dev,
				   unsigned int length, gfp_t gfp_mask)
{
	struct sk_buff *skb;
	skb = alloc_skb(length, gfp_mask);
	if (likely(skb))
		skb->dev = dev;
	return skb;
}

unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
	unsigned char *tmp = skb_tail_pointer(skb);
	skb->tail += len;
	skb->len += len;
	if (unlikely(skb->tail > skb->end))
		skb_over_panic(skb, len, __builtin_return_address(0));
	return tmp;
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	unsigned long flags = 0;
	struct sk_buff *result;

	spin_lock_irqsave(&list->lock, flags);
	result = __skb_dequeue(list);
	spin_unlock_irqrestore(&list->lock, flags);
	return result;
}

struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t priority)
{
	int headerlen = skb->data - skb->head;
	/*
	 *      Allocate the copy buffer
	 */
	struct sk_buff *n;

	n = alloc_skb(skb->end - skb->head, 0);
	if (!n)
		return NULL;

	/* Set the data pointer */
	n->tail = n->data = n->head + headerlen;

	/* Set the tail pointer and length */
	skb_put(n, skb->len);

	memcpy(n->head, skb->head, skb->end - skb->head);
	n->dev = skb->dev;

	return n;
}

struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority)
{
	return skb_copy(skb, priority);
}

struct sk_buff *skb_hclone(struct sk_buff *skb, void *pdata, unsigned int len)
{
	struct sk_buff *skb1 = (void *)malloc(sizeof(struct sk_buff_hclones));
	struct sk_buff_hclones *hclones = (struct sk_buff_hclones *)skb1;

	if (skb1 == NULL) {
		return NULL;
	}

	memset(skb1, 0, sizeof(struct sk_buff_hclones));
	skb1->head = skb1->data = (unsigned char *)pdata;
	skb_reset_tail_pointer(skb1);
	skb1->end = (uint8_t *)pdata + len;
	skb1->truesize = skb1->len = skb1->data_len = len;
	hclones->pskb2 = skb;
	skb1->flags |= SKBUF_FLAG_IS_HCLONE;

	taskENTER_CRITICAL();
	skb->hclone_ref++;
	taskEXIT_CRITICAL();

	return skb1;
}

struct sk_buff *pskb_copy(struct sk_buff *skb, gfp_t gfp_mask)
{
	asm volatile("nop; .word 0x1000ffff; nop; nop;");
	return NULL;
}

unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
	skb->data -= len;
	skb->len += len;
	if (unlikely(skb->data < skb->head))
		skb_under_panic(skb, len, __builtin_return_address(0));
	return skb->data;
}

unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
	return skb_pull_inline(skb, len);
}

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&list->lock, flags);
	__skb_queue_tail(list, newsk);
	spin_unlock_irqrestore(&list->lock, flags);
}

int skb_copy_bits(const struct sk_buff *skb, int offset, void *to, int length)
{
	int copy;

	if (offset > (int)skb->len - length)
		return -EFAULT;

	copy = skb->len - offset;
	if (copy > length)
		memcpy(to, skb->data + offset, length);
	else
		memcpy(to, skb->data + offset, copy);

	return 0;
}


/* Make private copy of skb with writable head and some headroom */

struct sk_buff *skb_realloc_headroom(struct sk_buff *skb, unsigned int headroom)
{
	// struct sk_buff *skb2;
	// int delta = headroom - skb_headroom(skb);

	// if (delta <= 0)
	// 	skb2 = pskb_copy(skb, GFP_ATOMIC);  //TODO: hichp should add pskb_copy/skb_clone
	// else {
	// 	skb2 = skb_clone(skb, GFP_ATOMIC);
	// 	if (skb2 && pskb_expand_head(skb2, SKB_DATA_ALIGN(delta), 0,
	// 				     GFP_ATOMIC)) {
	// 		kfree_skb(skb2);
	// 		skb2 = NULL;
	// 	}
	// }
	// return skb2;
	return NULL;
}

/**
 *	skb_trim - remove end from a buffer
 *	@skb: buffer to alter
 *	@len: new length
 *
 *	Cut the length of a buffer down by removing data from the tail. If
 *	the buffer is already under the length specified it is not modified.
 *	The skb must be linear.
 */
void skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (skb->len > len)
		__skb_trim(skb, len);
}
 
struct sk_buff *skb_copy_expand(struct sk_buff *skb,
				int newheadroom, int newtailroom,
				gfp_t gfp_mask)
{
	/*
	 *	Allocate the copy buffer
	 */
	struct sk_buff *n = alloc_skb(newheadroom + skb->len + newtailroom, GFP_ATOMIC);
	int oldheadroom = skb_headroom(skb);
	int head_copy_len, head_copy_off;

	if (!n)
		return NULL;

	skb_reserve(n, newheadroom);

	/* Set the tail pointer and length */
	skb_put(n, skb->len);

	head_copy_len = oldheadroom;
	head_copy_off = 0;
	if (newheadroom <= head_copy_len)
		head_copy_len = newheadroom;
	else
		head_copy_off = newheadroom - head_copy_len;

	/* Copy the linear header and data. */
	if (skb_copy_bits(skb, -head_copy_len, n->head + head_copy_off,
			  skb->len + head_copy_len))
		BUG();

	// copy_skb_header(n, skb);

	// skb_headers_offset_update(n, newheadroom - oldheadroom);

	return n;
}
