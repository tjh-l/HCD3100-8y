#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>

struct mod_init {
	const char *name;
	int  (*init)(void);
	int  (*exit)(void);
	bool initialized;
};

#define initcall_merge_body(a, b) a ## b
#define initcall_merge(x, y) initcall_merge_body(x, y)
#define ____define_initcall(n, f1, f2, layer, clayer, priority, uniqueid) \
static __attribute__((section(".dynload_rodata"))) const char initcall_merge(mod_name_##layer##_##n, uniqueid)[] = #n; \
static __attribute__((__used__)) __attribute__ ((section(".initcall." clayer #priority ".init"))) struct mod_init initcall_merge(mod_init_##layer##_##n, uniqueid) = { \
	.name = initcall_merge(mod_name_##layer##_##n, uniqueid), \
	.init = f1, \
	.exit = f2, \
	.initialized = false \
};

#define __define_initcall(n, f1, f2, layer, clayer, priority) \
	____define_initcall(n, f1, f2, layer, clayer, priority, __COUNTER__)

#define module_early(n, f1, f2, priority)                                    \
	__define_initcall(n, f1, f2, early, "early", priority)
#define module_core(n, f1, f2, priority)                                    \
	__define_initcall(n, f1, f2, core, "core", priority)
#define module_postcore(n, f1, f2, priority)                                \
	__define_initcall(n, f1, f2, postcore, "postcore", priority)
#define module_arch(n, f1, f2, priority)                                    \
	__define_initcall(n, f1, f2, arch, "arch", priority)
#define module_system(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, system, "system", priority)
#define module_driver(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, driver, "driver", priority)
#define module_driver_late(n, f1, f2, priority)                             \
	__define_initcall(n, f1, f2, driverlate, "driverlate", priority)
#define module_fs(n, f1, f2, priority)                                      \
	__define_initcall(n, f1, f2, fs, "fs", priority)
#define module_rootfs(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, rootfs, "rootfs", priority)
#define module_others(n, f1, f2, priority)                                  \
	__define_initcall(n, f1, f2, others, "others", priority)

#define __initcall(fn) module_others(fn, fn, NULL, 4)
#define __exitcall(fn) module_others(fn, NULL, fn, 4)

int module_init(const char *name);
int module_exit(const char *name);
int module_init2(const char *name, int n_exclude, char *excludes[]);
int module_exit2(const char *name, int n_exclude, char *excludes[]);

struct mod_info {
	const char *name;
	const char *info;
};

#define __module_info(n, str, uniqueid) \
static __attribute__((section(".dynload_rodata"))) const char initcall_merge(modinfo_name_##n, uniqueid)[] = #n; \
static __attribute__((section(".dynload_rodata"))) const char initcall_merge(modinfo_str_##n, uniqueid)[] = str; \
static __attribute__((__used__)) __attribute__ ((section(".modinfo.entry"))) struct mod_info initcall_merge(modinfo_init_##n, uniqueid) = { \
	.name = initcall_merge(modinfo_name_##n, uniqueid), \
	.info = initcall_merge(modinfo_str_##n, uniqueid), \
};

#define module_info(n, str) \
	__module_info(n, str, __COUNTER__) \

#ifdef __cplusplus
}
#endif

#endif
