/*
 *  linux/include/asm-arm/cpu-single.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * Single CPU
 */
#ifdef __STDC__
#define __catify_fn(name,x)	name##x
#else
#define __catify_fn(name,x)	name/**/x
#endif
#define __cpu_fn(name,x)	__catify_fn(name,x)

/*
 * If we are supporting multiple CPUs, then we must use a table of
 * function pointers for this lot.  Otherwise, we can optimise the
 * table away.
 */
#define cpu_proc_init			__cpu_fn(CPU_NAME,_proc_init)
#define cpu_proc_fin			__cpu_fn(CPU_NAME,_proc_fin)
#define cpu_reset			__cpu_fn(CPU_NAME,_reset)
#define cpu_do_idle			__cpu_fn(CPU_NAME,_do_idle)
#define cpu_dcache_clean_area		__cpu_fn(CPU_NAME,_dcache_clean_area)
#define cpu_do_switch_mm		__cpu_fn(CPU_NAME,_switch_mm)
#define cpu_set_pte			__cpu_fn(CPU_NAME,_set_pte)

#include <asm/page.h>

struct mm_struct;

/* declare all the functions as extern */
extern void cpu_proc_init(void);
extern void cpu_proc_fin(void);
extern int cpu_do_idle(void);
extern void cpu_dcache_clean_area(void *, int);
extern void cpu_do_switch_mm(unsigned long pgd_phys, struct mm_struct *mm);
extern void cpu_set_pte(pte_t *ptep, pte_t pte);
extern void cpu_reset(unsigned long addr) __attribute__((noreturn));
/* Exporting of Cache related API's */
#ifdef CONFIG_ARCH_CACTUS
extern void arm920_dma_inv_range(void *,void *);
extern void arm920_flush_user_cache_range(void *,void *,int); 
#endif
#if (defined(CONFIG_ARCH_PECOS) || defined(CONFIG_ARCH_NEVIS) || defined(CONFIG_ARCH_PECOS_256M))
extern void v6_dma_inv_range(void *,void *);
extern void v6_dma_clean_range(void *,void *,int); 
extern void v6_dma_flush_range(void *,void *,int); 
#endif 

#ifdef CONFIG_OUTER_CACHE
extern void l220_flush_all(void);
extern void l220_inv_all(void);
extern void l220_clean_all(void);
extern void l220_flush_range(unsigned long, unsigned long);
extern void l220_inv_range(unsigned long, unsigned long);
extern void l220_clean_range(unsigned long, unsigned long);
#endif
