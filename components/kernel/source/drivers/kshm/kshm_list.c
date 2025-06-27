#define LOG_TAG "MODULE"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <kernel/list.h>
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hcuapi/kshm.h>

#define ROUNDUP(a, b)	(((a) + ((b) - 1)) & ~((b) - 1))

typedef struct kshm_packet {
	struct list_head head;
	void *data;
	int size;
} kshm_packet;

typedef struct kshm_list_info {
	struct list_head packets;
	struct list_head reserved_packets;
	kshm_packet *new_packet;

	int rd_times;
	int wt_times;
	int num_packets;
	int data_size;
	int real_reserved;
	int cfg_reserved;

	int max_data_size;
	enum KSHM_DATA_DIRECTION data_direction; 
} kshm_list_info;

static inline void free_kshm_paket_list(struct list_head *packets)
{
	struct kshm_packet *pcur, *pnext;

	list_for_each_entry_safe (pcur, pnext, packets, head) {
		if (pcur) {
			list_del(&pcur->head);
			free(pcur);
		}
	}
}

kshm_handle_t kshm_create(size_t size)
{
	kshm_list_info *info;

	info = (kshm_list_info *)malloc(sizeof(kshm_list_info));
	if (!info) {
		return NULL;
	}
	memset(info, 0, sizeof(kshm_list_info));

	INIT_LIST_HEAD(&info->packets);
	INIT_LIST_HEAD(&info->reserved_packets);
	info->max_data_size = size;

	return info;
}

int kshm_set_data_direction(kshm_handle_t hdl, enum KSHM_DATA_DIRECTION data_direction)
{
	kshm_list_info *info = (kshm_list_info *)hdl;
	info->data_direction = data_direction;
	return KSHM_OK;
}

kshm_handle_t kshm_create_ext(size_t size, enum KSHM_DATA_DIRECTION data_direction)
{
	kshm_handle_t hdl = kshm_create(size);
	kshm_set_data_direction(hdl, data_direction);
	return hdl;
}

int kshm_destroy(kshm_handle_t hdl)
{
	kshm_list_info *info = (kshm_list_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	free_kshm_paket_list(&info->packets);
	free_kshm_paket_list(&info->reserved_packets);

	if (info->new_packet) {
		free(info->new_packet);
	}

	free(info);

	return KSHM_OK;
}

void *kshm_request_write(kshm_handle_t hdl, size_t size)
{
	kshm_list_info *info = (kshm_list_info *)hdl;
	kshm_packet *new_packet;

	if (!info || !size) {
		return NULL;
	}

	if (((int)size + info->data_size + info->real_reserved) > info->max_data_size) {
		return NULL;
	}

	new_packet = memalign(32, ROUNDUP(size + sizeof(kshm_packet), 32));
	if (!new_packet) {
		return NULL;
	}

	new_packet->data = ((void *)new_packet) + sizeof(kshm_packet);
	new_packet->size = size;
	info->new_packet = new_packet;

	return new_packet->data;
}

int kshm_update_write(kshm_handle_t hdl, size_t size)
{
	kshm_list_info *info = (kshm_list_info *)hdl;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	//printf("add pkt %p\n", info->new_packet);
	taskENTER_CRITICAL();
	if (!info->new_packet || (int)size != info->new_packet->size) {
		taskEXIT_CRITICAL();
		return KSHM_FAIL;
	}

	if ((info->data_direction == KSHM_TO_DEVICE ||
		 info->data_direction == KSHM_BIDIRECTIONAL) &&
		(info->wt_times % 2)) {
		vPortCacheFlush(info->new_packet->data, info->new_packet->size);
	}

	info->data_size += info->new_packet->size;
	list_add_tail(&info->new_packet->head, &info->packets);
	info->new_packet = NULL;
	info->num_packets++;
	info->wt_times++;
	taskEXIT_CRITICAL();

	return KSHM_OK;
}

int kshm_write(kshm_handle_t hdl, void *buf, size_t size)
{
	void *request;
	kshm_list_info *info = (kshm_list_info *)hdl;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	request = kshm_request_write(hdl, size);
	if (!request) {
		return KSHM_FAIL;
	}
	memcpy(request, buf, size);
	kshm_update_write(hdl, size);

	return size;
}

void *kshm_request_read(kshm_handle_t hdl, size_t size)
{
	kshm_packet *packet;
	kshm_list_info *info = (kshm_list_info *)hdl;

	if (!info || !size || list_empty(&info->packets)) {
		return NULL;
	}

	packet = list_first_entry(&info->packets, typeof(*packet), head);

	if(packet->size != (int)size) {
		printf("kshm read err, request size %d, packet size %d\n", (int)size, packet->size);
		return NULL;
	}

	if ((info->data_direction == KSHM_FROM_DEVICE ||
		info->data_direction == KSHM_BIDIRECTIONAL) &&
		(info->rd_times % 2)) {
		vPortCacheInvalidate(packet->data, size);
	}

	return packet->data;
}

int kshm_update_read(kshm_handle_t hdl, size_t size)
{
	kshm_list_info *info = (kshm_list_info *)hdl;
	kshm_packet *packet;

	taskENTER_CRITICAL();

	if (list_empty(&info->packets)) {
		taskEXIT_CRITICAL();
		return KSHM_FAIL;
	}

	packet = list_first_entry(&info->packets, typeof(*packet), head);
	if (info->cfg_reserved > 0) {
		struct kshm_packet *pcur, *pnext;
		int total_size = 0;

		if (packet) {
			list_del(&packet->head);
		}
		list_add(&packet->head, &info->reserved_packets);

		list_for_each_entry_safe (pcur, pnext, &info->reserved_packets, head) {
			if (pcur) {
				if (total_size < info->cfg_reserved) {
					total_size += pcur->size;
				} else {
					list_del(&pcur->head);
					free(pcur);
				}
			}
		}
		info->real_reserved = total_size;
	} else {
		if (packet) {
			list_del(&packet->head);
			free(packet);
		}
		if (info->real_reserved) {
			free_kshm_paket_list(&info->reserved_packets);
			info->real_reserved = 0;
		}
	}

	info->data_size -= size;
	info->num_packets--;
	info->rd_times++;
	taskEXIT_CRITICAL();
	//printf("del pkt %p\n", packet);

	return KSHM_OK;
}

int kshm_update_reserved(kshm_handle_t hdl, size_t size)
{
	kshm_list_info *info = (kshm_list_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	info->cfg_reserved = size;

	return KSHM_OK;
}

int kshm_read(kshm_handle_t hdl, void *buf, size_t size)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;
	void *request = NULL;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	request = kshm_request_read(hdl, size);
	if (!request) {
		return KSHM_FAIL;
	}

	memcpy(buf, request, size);

	kshm_update_read(hdl, size);

	return size;
}

int kshm_reset(kshm_handle_t hdl)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;

	if (!info) {
		return KSHM_OK;
	}

	info->data_size = 0;
	info->real_reserved = 0;
	info->cfg_reserved = 0;
	info->num_packets = 0;
	info->wt_times = 0;
	info->rd_times = 0;

	free_kshm_paket_list(&info->packets);
	free_kshm_paket_list(&info->reserved_packets);

	if (info->new_packet) {
		free(info->new_packet);
		info->new_packet = NULL;
	}

	return KSHM_OK;
}

int kshm_special_reset(kshm_handle_t hdl)
{
	int ret;

	taskENTER_CRITICAL();
	ret = kshm_reset(hdl);
	taskEXIT_CRITICAL();

	return ret;
}

size_t kshm_get_valid_size(kshm_handle_t hdl)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	return info->data_size;
}

size_t kshm_get_free_size(kshm_handle_t hdl)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	return info->max_data_size - info->data_size - info->real_reserved;
}

size_t kshm_get_total_size(kshm_handle_t hdl)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	return info->max_data_size;
}

int kshm_get_avail_packets(kshm_handle_t hdl)
{
	struct kshm_list_info *info = (struct kshm_list_info *)hdl;
	return info->num_packets >> 1;
}
