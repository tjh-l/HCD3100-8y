#ifndef __CPU_LEGACY_H
#define __CPU_LEGACY_H

extern int reset(void);
extern uint32_t get_processor_id(void);
extern void hw_watchdog_reset(unsigned long delay_us);
extern void hw_watchdog_disable(void);
extern int sys_hcprogrammer_check(void);
extern int sys_hcprogrammer_check_timeout(void);
extern unsigned long sys_time_from_boot(void);
extern void cache_flush(void *src, uint32_t len);
extern void cache_flush_invalidate(void *src, uint32_t len);
extern void cache_flush_all(void);
extern void cache_invalidate(void *src, uint32_t len);
extern void icache_invalidate(void *src, uint32_t len);
extern void irq_save_all(void);
extern void irq_restore_all(void);
extern void irq_save_av(void);
extern unsigned int __attribute__((noinline)) write_sync(void);
extern unsigned int is_amp(void);
extern int __cpu_clock_hz;
extern unsigned long arch_local_irq_save(void);
extern void arch_local_irq_restore(unsigned long flags);
extern unsigned long arch_local_irq_disable(void);
extern void arch_local_irq_enable(void);
void sys_strap_pin_ejtag_disable(void);
void sys_strap_pin_ejtag_enable(void);
#define get_cpu_clock() __cpu_clock_hz
#define flush_cache(a, b) cache_flush((void *)(a), (uint32_t)(b))

#define SDBBP()     asm volatile(".word 0x1000ffff; nop;");
#define sdbbp()     asm volatile(".word 0x1000ffff; nop;");

#endif
