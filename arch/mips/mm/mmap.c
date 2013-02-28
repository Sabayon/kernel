/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011 Wind River Systems,
 *   written by Ralf Baechle <ralf@linux-mips.org>
 */
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/personality.h>
#include <linux/random.h>
#include <linux/sched.h>

unsigned long shm_align_mask = PAGE_SIZE - 1;	/* Sane caches */
EXPORT_SYMBOL(shm_align_mask);

/* gap between mmap and stack */
#define MIN_GAP (128*1024*1024UL)
#define MAX_GAP ((TASK_SIZE)/6*5)

static int mmap_is_legacy(void)
{
	if (current->personality & ADDR_COMPAT_LAYOUT)
		return 1;

	if (rlimit(RLIMIT_STACK) == RLIM_INFINITY)
		return 1;

	return sysctl_legacy_va_layout;
}

static unsigned long mmap_base(unsigned long rnd)
{
	unsigned long gap = rlimit(RLIMIT_STACK);

	if (gap < MIN_GAP)
		gap = MIN_GAP;
	else if (gap > MAX_GAP)
		gap = MAX_GAP;

	return PAGE_ALIGN(TASK_SIZE - gap - rnd);
}

#define COLOUR_ALIGN(addr, pgoff)				\
	((((addr) + shm_align_mask) & ~shm_align_mask) +	\
	 (((pgoff) << PAGE_SHIFT) & shm_align_mask))

enum mmap_allocation_direction {UP, DOWN};

static unsigned long arch_get_unmapped_area_common(struct file *filp,
	unsigned long addr0, unsigned long len, unsigned long pgoff,
	unsigned long flags, enum mmap_allocation_direction dir)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long addr = addr0;
	int do_color_align;
	unsigned long offset = gr_rand_threadstack_offset(mm, filp, flags);
	struct vm_unmapped_area_info info;

	if (unlikely(len > TASK_SIZE))
		return -ENOMEM;

	if (flags & MAP_FIXED) {
		/* Even MAP_FIXED mappings must reside within TASK_SIZE */
		if (TASK_SIZE - len < addr)
			return -EINVAL;

		/*
		 * We do not accept a shared mapping if it would violate
		 * cache aliasing constraints.
		 */
		if ((flags & MAP_SHARED) &&
		    ((addr - (pgoff << PAGE_SHIFT)) & shm_align_mask))
			return -EINVAL;
		return addr;
	}

	do_color_align = 0;
	if (filp || (flags & MAP_SHARED))
		do_color_align = 1;

	/* requesting a specific address */

#ifdef CONFIG_PAX_RANDMMAP
	if (!(current->mm->pax_flags & MF_PAX_RANDMMAP))
#endif

	if (addr) {
		if (do_color_align)
			addr = COLOUR_ALIGN(addr, pgoff);
		else
			addr = PAGE_ALIGN(addr);

		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr && check_heap_stack_gap(vmm, addr, len, offset))
			return addr;
	}

	info.length = len;
	info.align_mask = do_color_align ? (PAGE_MASK & shm_align_mask) : 0;
	info.align_offset = pgoff << PAGE_SHIFT;

	if (dir == DOWN) {
		info.flags = VM_UNMAPPED_AREA_TOPDOWN;
		info.low_limit = PAGE_SIZE;
		info.high_limit = mm->mmap_base;
		addr = vm_unmapped_area(&info);

		if (!(addr & ~PAGE_MASK))
			return addr;

		/*
		 * A failed mmap() very likely causes application failure,
		 * so fall back to the bottom-up function here. This scenario
		 * can happen with large stack limits and large mmap()
		 * allocations.
		 */
	}

	info.flags = 0;
	info.low_limit = mm->mmap_base;
	info.high_limit = TASK_SIZE;
	return vm_unmapped_area(&info);
}

unsigned long arch_get_unmapped_area(struct file *filp, unsigned long addr0,
	unsigned long len, unsigned long pgoff, unsigned long flags)
{
	return arch_get_unmapped_area_common(filp,
			addr0, len, pgoff, flags, UP);
}

/*
 * There is no need to export this but sched.h declares the function as
 * extern so making it static here results in an error.
 */
unsigned long arch_get_unmapped_area_topdown(struct file *filp,
	unsigned long addr0, unsigned long len, unsigned long pgoff,
	unsigned long flags)
{
	return arch_get_unmapped_area_common(filp,
			addr0, len, pgoff, flags, DOWN);
}

void arch_pick_mmap_layout(struct mm_struct *mm)
{
	unsigned long random_factor = 0UL;

#ifdef CONFIG_PAX_RANDMMAP
	if (!(mm->pax_flags & MF_PAX_RANDMMAP))
#endif

	if (current->flags & PF_RANDOMIZE) {
		random_factor = get_random_int();
		random_factor = random_factor << PAGE_SHIFT;
		if (TASK_IS_32BIT_ADDR)
			random_factor &= 0xfffffful;
		else
			random_factor &= 0xffffffful;
	}

	if (mmap_is_legacy()) {
		mm->mmap_base = TASK_UNMAPPED_BASE + random_factor;

#ifdef CONFIG_PAX_RANDMMAP
		if (mm->pax_flags & MF_PAX_RANDMMAP)
			mm->mmap_base += mm->delta_mmap;
#endif

		mm->get_unmapped_area = arch_get_unmapped_area;
		mm->unmap_area = arch_unmap_area;
	} else {
		mm->mmap_base = mmap_base(random_factor);

#ifdef CONFIG_PAX_RANDMMAP
		if (mm->pax_flags & MF_PAX_RANDMMAP)
			mm->mmap_base -= mm->delta_mmap + mm->delta_stack;
#endif

		mm->get_unmapped_area = arch_get_unmapped_area_topdown;
		mm->unmap_area = arch_unmap_area_topdown;
	}
}

int __virt_addr_valid(const volatile void *kaddr)
{
	return pfn_valid(PFN_DOWN(virt_to_phys(kaddr)));
}
EXPORT_SYMBOL_GPL(__virt_addr_valid);
