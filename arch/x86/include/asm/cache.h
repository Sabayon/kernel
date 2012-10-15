#ifndef _ASM_X86_CACHE_H
#define _ASM_X86_CACHE_H

#include <linux/linkage.h>

/* L1 cache line size */
#define L1_CACHE_SHIFT	(CONFIG_X86_L1_CACHE_SHIFT)
#define L1_CACHE_BYTES	(_AC(1,UL) << L1_CACHE_SHIFT)

#define __read_mostly __attribute__((__section__(".data..read_mostly")))
#define __read_only __attribute__((__section__(".data..read_only")))

#define INTERNODE_CACHE_SHIFT CONFIG_X86_INTERNODE_CACHE_SHIFT
#define INTERNODE_CACHE_BYTES (_AC(1,UL) << INTERNODE_CACHE_SHIFT)

#ifdef CONFIG_X86_VSMP
#ifdef CONFIG_SMP
#define __cacheline_aligned_in_smp					\
	__attribute__((__aligned__(INTERNODE_CACHE_BYTES)))		\
	__page_aligned_data
#endif
#endif

#endif /* _ASM_X86_CACHE_H */
