ifeq ($(CONFIG_KSHM_DYNAMIC_BUFFER_LIST_MODE),y)
kshm-$(CONFIG_KSHM_DYNAMIC_BUFFER_LIST_MODE) = kshm_list.o
else
kshm-y = kshm.o
endif

obj-$(CONFIG_DRV_KSHM) += kshm.o

