/*
 *  mm/mprotect.c
 *
 *  (C) Copyright 1994 Linus Torvalds
 *  (C) Copyright 2002 Christoph Hellwig
 *
 *  Address space accounting code	<alan@lxorguk.ukuu.org.uk>
 *  (C) Copyright 2002 Red Hat Inc, All Rights Reserved
 */

#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/security.h>
#include <linux/mempolicy.h>
#include <linux/personality.h>
#include <linux/syscalls.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/mmu_notifier.h>
#include <linux/migrate.h>
#include <linux/perf_event.h>

#ifdef CONFIG_PAX_MPROTECT
#include <linux/elf.h>
#include <linux/binfmts.h>
#endif

#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>

#ifndef pgprot_modify
static inline pgprot_t pgprot_modify(pgprot_t oldprot, pgprot_t newprot)
{
	return newprot;
}
#endif

static void change_pte_range(struct mm_struct *mm, pmd_t *pmd,
		unsigned long addr, unsigned long end, pgprot_t newprot,
		int dirty_accountable)
{
	pte_t *pte, oldpte;
	spinlock_t *ptl;

	pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	arch_enter_lazy_mmu_mode();
	do {
		oldpte = *pte;
		if (pte_present(oldpte)) {
			pte_t ptent;

			ptent = ptep_modify_prot_start(mm, addr, pte);
			ptent = pte_modify(ptent, newprot);

			/*
			 * Avoid taking write faults for pages we know to be
			 * dirty.
			 */
			if (dirty_accountable && pte_dirty(ptent))
				ptent = pte_mkwrite(ptent);

			ptep_modify_prot_commit(mm, addr, pte, ptent);
		} else if (IS_ENABLED(CONFIG_MIGRATION) && !pte_file(oldpte)) {
			swp_entry_t entry = pte_to_swp_entry(oldpte);

			if (is_write_migration_entry(entry)) {
				/*
				 * A protection check is difficult so
				 * just be safe and disable write
				 */
				make_migration_entry_read(&entry);
				set_pte_at(mm, addr, pte,
					swp_entry_to_pte(entry));
			}
		}
	} while (pte++, addr += PAGE_SIZE, addr != end);
	arch_leave_lazy_mmu_mode();
	pte_unmap_unlock(pte - 1, ptl);
}

static inline void change_pmd_range(struct vm_area_struct *vma, pud_t *pud,
		unsigned long addr, unsigned long end, pgprot_t newprot,
		int dirty_accountable)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_trans_huge(*pmd)) {
			if (next - addr != HPAGE_PMD_SIZE)
				split_huge_page_pmd(vma->vm_mm, pmd);
			else if (change_huge_pmd(vma, pmd, addr, newprot))
				continue;
			/* fall through */
		}
		if (pmd_none_or_clear_bad(pmd))
			continue;
		change_pte_range(vma->vm_mm, pmd, addr, next, newprot,
				 dirty_accountable);
	} while (pmd++, addr = next, addr != end);
}

static inline void change_pud_range(struct vm_area_struct *vma, pgd_t *pgd,
		unsigned long addr, unsigned long end, pgprot_t newprot,
		int dirty_accountable)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_offset(pgd, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_none_or_clear_bad(pud))
			continue;
		change_pmd_range(vma, pud, addr, next, newprot,
				 dirty_accountable);
	} while (pud++, addr = next, addr != end);
}

static void change_protection(struct vm_area_struct *vma,
		unsigned long addr, unsigned long end, pgprot_t newprot,
		int dirty_accountable)
{
	struct mm_struct *mm = vma->vm_mm;
	pgd_t *pgd;
	unsigned long next;
	unsigned long start = addr;

	BUG_ON(addr >= end);
	pgd = pgd_offset(mm, addr);
	flush_cache_range(vma, addr, end);
	do {
		next = pgd_addr_end(addr, end);
		if (pgd_none_or_clear_bad(pgd))
			continue;
		change_pud_range(vma, pgd, addr, next, newprot,
				 dirty_accountable);
	} while (pgd++, addr = next, addr != end);
	flush_tlb_range(vma, start, end);
}

#ifdef CONFIG_ARCH_TRACK_EXEC_LIMIT
/* called while holding the mmap semaphor for writing except stack expansion */
void track_exec_limit(struct mm_struct *mm, unsigned long start, unsigned long end, unsigned long prot)
{
	unsigned long oldlimit, newlimit = 0UL;

	if (!(mm->pax_flags & MF_PAX_PAGEEXEC) || (__supported_pte_mask & _PAGE_NX))
		return;

	spin_lock(&mm->page_table_lock);
	oldlimit = mm->context.user_cs_limit;
	if ((prot & VM_EXEC) && oldlimit < end)
		/* USER_CS limit moved up */
		newlimit = end;
	else if (!(prot & VM_EXEC) && start < oldlimit && oldlimit <= end)
		/* USER_CS limit moved down */
		newlimit = start;

	if (newlimit) {
		mm->context.user_cs_limit = newlimit;

#ifdef CONFIG_SMP
		wmb();
		cpus_clear(mm->context.cpu_user_cs_mask);
		cpu_set(smp_processor_id(), mm->context.cpu_user_cs_mask);
#endif

		set_user_cs(mm->context.user_cs_base, mm->context.user_cs_limit, smp_processor_id());
	}
	spin_unlock(&mm->page_table_lock);
	if (newlimit == end) {
		struct vm_area_struct *vma = find_vma(mm, oldlimit);

		for (; vma && vma->vm_start < end; vma = vma->vm_next)
			if (is_vm_hugetlb_page(vma))
				hugetlb_change_protection(vma, vma->vm_start, vma->vm_end, vma->vm_page_prot);
			else
				change_protection(vma, vma->vm_start, vma->vm_end, vma->vm_page_prot, vma_wants_writenotify(vma));
	}
}
#endif

int
mprotect_fixup(struct vm_area_struct *vma, struct vm_area_struct **pprev,
	unsigned long start, unsigned long end, unsigned long newflags)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long oldflags = vma->vm_flags;
	long nrpages = (end - start) >> PAGE_SHIFT;
	unsigned long charged = 0;
	pgoff_t pgoff;
	int error;
	int dirty_accountable = 0;

#ifdef CONFIG_PAX_SEGMEXEC
	struct vm_area_struct *vma_m = NULL;
	unsigned long start_m, end_m;

	start_m = start + SEGMEXEC_TASK_SIZE;
	end_m = end + SEGMEXEC_TASK_SIZE;
#endif

	if (newflags == oldflags) {
		*pprev = vma;
		return 0;
	}

	if (newflags & (VM_READ | VM_WRITE | VM_EXEC)) {
		struct vm_area_struct *prev = vma->vm_prev, *next = vma->vm_next;

		if (next && (next->vm_flags & VM_GROWSDOWN) && sysctl_heap_stack_gap > next->vm_start - end)
			return -ENOMEM;

		if (prev && (prev->vm_flags & VM_GROWSUP) && sysctl_heap_stack_gap > start - prev->vm_end)
			return -ENOMEM;
	}

	/*
	 * If we make a private mapping writable we increase our commit;
	 * but (without finer accounting) cannot reduce our commit if we
	 * make it unwritable again. hugetlb mapping were accounted for
	 * even if read-only so there is no need to account for them here
	 */
	if (newflags & VM_WRITE) {
		if (!(oldflags & (VM_ACCOUNT|VM_WRITE|VM_HUGETLB|
						VM_SHARED|VM_NORESERVE))) {
			charged = nrpages;
			if (security_vm_enough_memory_mm(mm, charged))
				return -ENOMEM;
			newflags |= VM_ACCOUNT;
		}
	}

#ifdef CONFIG_PAX_SEGMEXEC
	if ((mm->pax_flags & MF_PAX_SEGMEXEC) && ((oldflags ^ newflags) & VM_EXEC)) {
		if (start != vma->vm_start) {
			error = split_vma(mm, vma, start, 1);
			if (error)
				goto fail;
			BUG_ON(!*pprev || (*pprev)->vm_next == vma);
			*pprev = (*pprev)->vm_next;
		}

		if (end != vma->vm_end) {
			error = split_vma(mm, vma, end, 0);
			if (error)
				goto fail;
		}

		if (pax_find_mirror_vma(vma)) {
			error = __do_munmap(mm, start_m, end_m - start_m);
			if (error)
				goto fail;
		} else {
			vma_m = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
			if (!vma_m) {
				error = -ENOMEM;
				goto fail;
			}
			vma->vm_flags = newflags;
			error = pax_mirror_vma(vma_m, vma);
			if (error) {
				vma->vm_flags = oldflags;
				goto fail;
			}
		}
	}
#endif

	/*
	 * First try to merge with previous and/or next vma.
	 */
	pgoff = vma->vm_pgoff + ((start - vma->vm_start) >> PAGE_SHIFT);
	*pprev = vma_merge(mm, *pprev, start, end, newflags,
			vma->anon_vma, vma->vm_file, pgoff, vma_policy(vma));
	if (*pprev) {
		vma = *pprev;
		goto success;
	}

	*pprev = vma;

	if (start != vma->vm_start) {
		error = split_vma(mm, vma, start, 1);
		if (error)
			goto fail;
	}

	if (end != vma->vm_end) {
		error = split_vma(mm, vma, end, 0);
		if (error)
			goto fail;
	}

success:
	/*
	 * vm_flags and vm_page_prot are protected by the mmap_sem
	 * held in write mode.
	 */

#ifdef CONFIG_PAX_SEGMEXEC
	if ((mm->pax_flags & MF_PAX_SEGMEXEC) && (newflags & VM_EXEC) && ((vma->vm_flags ^ newflags) & VM_READ))
		pax_find_mirror_vma(vma)->vm_flags ^= VM_READ;
#endif

	vma->vm_flags = newflags;

#ifdef CONFIG_PAX_MPROTECT
	if (mm->binfmt && mm->binfmt->handle_mprotect)
		mm->binfmt->handle_mprotect(vma, newflags);
#endif

	vma->vm_page_prot = pgprot_modify(vma->vm_page_prot,
					  vm_get_page_prot(vma->vm_flags));

	if (vma_wants_writenotify(vma)) {
		vma->vm_page_prot = vm_get_page_prot(newflags & ~VM_SHARED);
		dirty_accountable = 1;
	}

	mmu_notifier_invalidate_range_start(mm, start, end);
	if (is_vm_hugetlb_page(vma))
		hugetlb_change_protection(vma, start, end, vma->vm_page_prot);
	else
		change_protection(vma, start, end, vma->vm_page_prot, dirty_accountable);
	mmu_notifier_invalidate_range_end(mm, start, end);
	vm_stat_account(mm, oldflags, vma->vm_file, -nrpages);
	vm_stat_account(mm, newflags, vma->vm_file, nrpages);
	perf_event_mmap(vma);
	return 0;

fail:
	vm_unacct_memory(charged);
	return error;
}

SYSCALL_DEFINE3(mprotect, unsigned long, start, size_t, len,
		unsigned long, prot)
{
	unsigned long vm_flags, nstart, end, tmp, reqprot;
	struct vm_area_struct *vma, *prev;
	int error = -EINVAL;
	const int grows = prot & (PROT_GROWSDOWN|PROT_GROWSUP);
	prot &= ~(PROT_GROWSDOWN|PROT_GROWSUP);
	if (grows == (PROT_GROWSDOWN|PROT_GROWSUP)) /* can't be both */
		return -EINVAL;

	if (start & ~PAGE_MASK)
		return -EINVAL;
	if (!len)
		return 0;
	len = PAGE_ALIGN(len);
	end = start + len;
	if (end <= start)
		return -ENOMEM;

#ifdef CONFIG_PAX_SEGMEXEC
	if (current->mm->pax_flags & MF_PAX_SEGMEXEC) {
		if (end > SEGMEXEC_TASK_SIZE)
			return -EINVAL;
	} else
#endif

	if (end > TASK_SIZE)
		return -EINVAL;

	if (!arch_validate_prot(prot))
		return -EINVAL;

	reqprot = prot;
	/*
	 * Does the application expect PROT_READ to imply PROT_EXEC:
	 */
	if ((prot & (PROT_READ | PROT_WRITE)) && (current->personality & READ_IMPLIES_EXEC))
		prot |= PROT_EXEC;

	vm_flags = calc_vm_prot_bits(prot);

	down_write(&current->mm->mmap_sem);

	vma = find_vma(current->mm, start);
	error = -ENOMEM;
	if (!vma)
		goto out;
	prev = vma->vm_prev;
	if (unlikely(grows & PROT_GROWSDOWN)) {
		if (vma->vm_start >= end)
			goto out;
		start = vma->vm_start;
		error = -EINVAL;
		if (!(vma->vm_flags & VM_GROWSDOWN))
			goto out;
	}
	else {
		if (vma->vm_start > start)
			goto out;
		if (unlikely(grows & PROT_GROWSUP)) {
			end = vma->vm_end;
			error = -EINVAL;
			if (!(vma->vm_flags & VM_GROWSUP))
				goto out;
		}
	}
	if (start > vma->vm_start)
		prev = vma;

#ifdef CONFIG_PAX_MPROTECT
	if (current->mm->binfmt && current->mm->binfmt->handle_mprotect)
		current->mm->binfmt->handle_mprotect(vma, vm_flags);
#endif

	for (nstart = start ; ; ) {
		unsigned long newflags;

		/* Here we know that  vma->vm_start <= nstart < vma->vm_end. */

		newflags = vm_flags | (vma->vm_flags & ~(VM_READ | VM_WRITE | VM_EXEC));

		/* newflags >> 4 shift VM_MAY% in place of VM_% */
		if ((newflags & ~(newflags >> 4)) & (VM_READ | VM_WRITE | VM_EXEC)) {
			if (prot & (PROT_WRITE | PROT_EXEC))
				gr_log_rwxmprotect(vma->vm_file);

			error = -EACCES;
			goto out;
		}

		if (!gr_acl_handle_mprotect(vma->vm_file, prot)) {
			error = -EACCES;
			goto out;
		}

		error = security_file_mprotect(vma, reqprot, prot);
		if (error)
			goto out;

		tmp = vma->vm_end;
		if (tmp > end)
			tmp = end;
		error = mprotect_fixup(vma, &prev, nstart, tmp, newflags);
		if (error)
			goto out;

		track_exec_limit(current->mm, nstart, tmp, vm_flags);

		nstart = tmp;

		if (nstart < prev->vm_end)
			nstart = prev->vm_end;
		if (nstart >= end)
			goto out;

		vma = prev->vm_next;
		if (!vma || vma->vm_start != nstart) {
			error = -ENOMEM;
			goto out;
		}
	}
out:
	up_write(&current->mm->mmap_sem);
	return error;
}
