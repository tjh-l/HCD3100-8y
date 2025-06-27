lib-y := display_options.o
lib-y := uart.o
lib-y += stub.o ctype.o printf.o
lib-y += ../../../../kernel/source/lib/fdt/fdt_api.o
lib-y += ../../../../kernel/source/lib/fdt/fdt.o
lib-y += ../../../../kernel/source/lib/fdt/fdt_ro.o
lib-y += ../../../../kernel/source/lib/fdt/fdt_rw.o
lib-y += ../../../../kernel/source/drivers/clock/hc_clk_gate.o
lib-y += ../../../../kernel/source/drivers/gpio/gpio.o
lib-y += ../../../../kernel/source/drivers/pinmux/pinmux.o
lib-y += ../../../../kernel/source/lib/libc/alloc.o
lib-y += ../../../../kernel/source/lib/libc/mallocr.o
lib-y += ../../../../kernel/source/lib/libc/freer.o
lib-y += ../../../../kernel/source/lib/libc/callocr.o
lib-y += ../../../../kernel/source/lib/libc/reallocr.o
lib-y += ../../../../kernel/source/lib/libc/isatty.o
lib-y += ../../../../kernel/source/lib/crc/crc32.o
lib-y += ../../../../kernel/source/arch/mips/cpu.o
lib-y += ../../../../kernel/source/arch/mips/init_hook.o
lib-y += ../../../../kernel/source/arch/mips/hcprogrammer.o

CFLAGS_printf.o += -Wno-implicit-fallthrough

ifeq ($(CONFIG_MINIBOOT_LIB_LZMA),y)
obj-y += ../../../../kernel/source/lib/lzma/
endif

ifeq ($(CONFIG_MINIBOOT_LIB_GZIP),y)
obj-y += ../../../../kernel/source/lib/zlib/
endif

ifeq ($(CONFIG_SOC_HC15XX),y)
obj-y += ../../../../kernel/source/arch/mips/mips32r1/
else ifeq ($(CONFIG_SOC_HC16XX),y)
obj-y += ../../../../kernel/source/arch/mips/mips74kef/
endif
$(obj)/fdt_api.o: $(DTS).dtb

cflags-y += -DCONFIG_OF_EMBED
cflags-y += -DCONFIG_DEFAULT_DEVICE_TREE=\"$(CONFIG_MINIBOOT_DEFAULT_DEVICE_TREE)\"
cflags-y += -Wno-sign-compare
cflags-y += -DCONFIG_MEM_NR_MMZ=1

obj-y += lib.o
