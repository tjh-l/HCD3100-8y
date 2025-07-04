/*
 * platform_device.h - generic, centralized driver model
 *
 * Copyright (c) 2001-2003 Patrick Mochel <mochel@osdl.org>
 *
 * This file is released under the GPLv2
 *
 * See Documentation/driver-model/ for more information.
 */

#ifndef _PLATFORM_DEVICE_H_
#define _PLATFORM_DEVICE_H_

#include <kernel/list.h>
#include <linux/device.h>

#define PLATFORM_DEVID_NONE	(-1)
#define PLATFORM_DEVID_AUTO	(-2)

struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;

	const struct platform_device_id	*id_entry;
};

#define platform_get_device_id(pdev)	((pdev)->id_entry)

#define to_platform_device(x) container_of((x), struct platform_device, dev)

extern int platform_device_register(struct platform_device *);
extern void platform_device_unregister(struct platform_device *);

extern struct bus_type platform_bus_type;
extern struct device platform_bus;

extern void arch_setup_pdev_archdata(struct platform_device *);
extern struct resource *platform_get_resource(struct platform_device *,
					      unsigned int, unsigned int);
extern int platform_get_irq(struct platform_device *, unsigned int);
extern struct resource *platform_get_resource_byname(struct platform_device *,
						     unsigned int,
						     const char *);
extern int platform_get_irq_byname(struct platform_device *, const char *);
extern int platform_add_devices(struct platform_device **, int);

struct platform_device_info {
		struct device *parent;

		const char *name;
		int id;

		const struct resource *res;
		unsigned int num_res;

		const void *data;
		size_t size_data;
		u64 dma_mask;
};
extern struct platform_device *platform_device_register_full(
		const struct platform_device_info *pdevinfo);
extern struct platform_device *of_platform_device_register_full(
		const struct platform_device_info *pdevinfo, struct device_node *np);
int platform_device_probe_resources(struct platform_device *dev, struct device_node *np);

/**
 * platform_device_register_resndata - add a platform-level device with
 * resources and platform-specific data
 *
 * @parent: parent device for the device we're adding
 * @name: base name of the device we're adding
 * @id: instance id
 * @res: set of resources that needs to be allocated for the device
 * @num: number of resources
 * @data: platform specific data for this platform device
 * @size: size of platform specific data
 *
 * Returns &struct platform_device pointer on success, or ERR_PTR() on error.
 */
static inline struct platform_device *platform_device_register_resndata(
		struct device *parent, const char *name, int id,
		const struct resource *res, unsigned int num,
		const void *data, size_t size) {

	struct platform_device_info pdevinfo = {
		.parent = parent,
		.name = name,
		.id = id,
		.res = res,
		.num_res = num,
		.data = data,
		.size_data = size,
		.dma_mask = 0,
	};

	return platform_device_register_full(&pdevinfo);
}

static inline struct platform_device *of_platform_device_register_resndata(
		struct device *parent, const char *name, int id,
		const struct resource *res, unsigned int num,
		const void *data, size_t size, struct device_node *np) {

	struct platform_device_info pdevinfo = {
		.parent = parent,
		.name = name,
		.id = id,
		.res = res,
		.num_res = num,
		.data = data,
		.size_data = size,
		.dma_mask = 0,
	};

	return of_platform_device_register_full(&pdevinfo, np);
}

/**
 * platform_device_register_simple - add a platform-level device and its resources
 * @name: base name of the device we're adding
 * @id: instance id
 * @res: set of resources that needs to be allocated for the device
 * @num: number of resources
 *
 * This function creates a simple platform device that requires minimal
 * resource and memory management. Canned release function freeing memory
 * allocated for the device allows drivers using such devices to be
 * unloaded without waiting for the last reference to the device to be
 * dropped.
 *
 * This interface is primarily intended for use with legacy drivers which
 * probe hardware directly.  Because such drivers create sysfs device nodes
 * themselves, rather than letting system infrastructure handle such device
 * enumeration tasks, they don't fully conform to the Linux driver model.
 * In particular, when such drivers are built as modules, they can't be
 * "hotplugged".
 *
 * Returns &struct platform_device pointer on success, or ERR_PTR() on error.
 */
static inline struct platform_device *platform_device_register_simple(
		const char *name, int id,
		const struct resource *res, unsigned int num)
{
	return platform_device_register_resndata(NULL, name, id,
			res, num, NULL, 0);
}

static inline struct platform_device *of_platform_device_register_simple(
		struct device_node *np,
		const char *name, int id,
		const struct resource *res, unsigned int num)
{
	return of_platform_device_register_resndata(NULL, name, id,
			res, num, NULL, 0, np);
}

/**
 * platform_device_register_data - add a platform-level device with platform-specific data
 * @parent: parent device for the device we're adding
 * @name: base name of the device we're adding
 * @id: instance id
 * @data: platform specific data for this platform device
 * @size: size of platform specific data
 *
 * This function creates a simple platform device that requires minimal
 * resource and memory management. Canned release function freeing memory
 * allocated for the device allows drivers using such devices to be
 * unloaded without waiting for the last reference to the device to be
 * dropped.
 *
 * Returns &struct platform_device pointer on success, or ERR_PTR() on error.
 */
static inline struct platform_device *platform_device_register_data(
		struct device *parent, const char *name, int id,
		const void *data, size_t size)
{
	return platform_device_register_resndata(parent, name, id,
			NULL, 0, data, size);
}

extern struct platform_device *platform_device_alloc(const char *name, int id);
extern int platform_device_add_resources(struct platform_device *pdev,
					 const struct resource *res,
					 unsigned int num);
extern int platform_device_add_data(struct platform_device *pdev,
				    const void *data, size_t size);
extern int platform_device_add(struct platform_device *pdev);
extern void platform_device_del(struct platform_device *pdev);
extern void platform_device_put(struct platform_device *pdev);
extern struct platform_device *of_find_device_by_node(struct device_node *np);

struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};

extern int platform_driver_probe(struct platform_driver *driver,
		int (*probe)(struct platform_device *));

#define to_platform_driver(drv)	(container_of((drv), struct platform_driver, \
				 driver))

extern int platform_driver_register(struct platform_driver *);
extern void platform_driver_unregister(struct platform_driver *);

extern int platform_driver_probe(struct platform_driver *driver,
		int (*probe)(struct platform_device *));

static inline void *platform_get_drvdata(const struct platform_device *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

static inline void platform_set_drvdata(struct platform_device *pdev,
					void *data)
{
	dev_set_drvdata(&pdev->dev, data);
}

/* module_platform_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_platform_driver(__platform_driver) \
	linux_module_driver(__platform_driver, platform_driver_register, \
			platform_driver_unregister)

/* builtin_platform_driver() - Helper macro for builtin drivers that
 * don't do anything special in driver init.  This eliminates some
 * boilerplate.  Each driver may only use this macro once, and
 * calling it replaces device_initcall().  Note this is meant to be
 * a parallel of module_platform_driver() above, but w/o _exit stuff.
 */
#define builtin_platform_driver(__platform_driver) \
	builtin_driver(__platform_driver, platform_driver_register)

/* module_platform_driver_probe() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
__initcall(__platform_driver##_init); \
static void __exit __platform_driver##_exit(void) \
{ \
	platform_driver_unregister(&(__platform_driver)); \
} \
__exitcall(__platform_driver##_exit);

/* builtin_platform_driver_probe() - Helper macro for drivers that don't do
 * anything special in device init.  This eliminates some boilerplate.  Each
 * driver may only use this macro once, and using it replaces device_initcall.
 * This is meant to be a parallel of module_platform_driver_probe above, but
 * without the __exit parts.
 */
#define builtin_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
device_initcall(__platform_driver##_init); \

int platform_register_drivers(struct platform_driver * const *drivers,
				unsigned int count);
void platform_unregister_drivers(struct platform_driver * const *drivers,
				 unsigned int count);

#define platform_pm_suspend		NULL
#define platform_pm_resume		NULL

#define platform_pm_freeze		NULL
#define platform_pm_thaw		NULL
#define platform_pm_poweroff		NULL
#define platform_pm_restore		NULL

#define USE_PLATFORM_PM_SLEEP_OPS

#endif /* _PLATFORM_DEVICE_H_ */
