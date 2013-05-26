/**
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>

#include <mach/irqs.h>

#include <mach/platform.h>
#include <mach/vcio.h>

#define TOTAL_SLOTS (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)

#define VCHIQ_DOORBELL_IRQ IRQ_ARM_DOORBELL_0
#define VCHIQ_ARM_ADDRESS(x) ((void *)__virt_to_bus((unsigned)x))

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_connected.h"

#define MAX_FRAGMENTS (VCHIQ_NUM_CURRENT_BULKS * 2)

typedef struct vchiq_2835_state_struct {
   int inited;
   VCHIQ_ARM_STATE_T arm_state;
} VCHIQ_2835_ARM_STATE_T;

static char *g_slot_mem;
static int g_slot_mem_size;
dma_addr_t g_slot_phys;
static FRAGMENTS_T *g_fragments_base;
static FRAGMENTS_T *g_free_fragments;
struct semaphore g_free_fragments_sema;

extern int vchiq_arm_log_level;

static DEFINE_SEMAPHORE(g_free_fragments_mutex);

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id);

static int
create_pagelist(char __user *buf, size_t count, unsigned short type,
                struct task_struct *task, PAGELIST_T ** ppagelist);

static void
free_pagelist(PAGELIST_T *pagelist, int actual);

int __init
vchiq_platform_init(VCHIQ_STATE_T *state)
{
	VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
	int frag_mem_size;
	int err;
	int i;

	/* Allocate space for the channels in coherent memory */
	g_slot_mem_size = PAGE_ALIGN(TOTAL_SLOTS * VCHIQ_SLOT_SIZE);
	frag_mem_size = PAGE_ALIGN(sizeof(FRAGMENTS_T) * MAX_FRAGMENTS);

	g_slot_mem = dma_alloc_coherent(NULL, g_slot_mem_size + frag_mem_size,
		&g_slot_phys, GFP_ATOMIC);

	if (!g_slot_mem) {
		vchiq_log_error(vchiq_arm_log_level,
			"Unable to allocate channel memory");
		err = -ENOMEM;
		goto failed_alloc;
	}

	WARN_ON(((int)g_slot_mem & (PAGE_SIZE - 1)) != 0);

	vchiq_slot_zero = vchiq_init_slots(g_slot_mem, g_slot_mem_size);
	if (!vchiq_slot_zero) {
		err = -EINVAL;
		goto failed_init_slots;
	}

	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX] =
		(int)g_slot_phys + g_slot_mem_size;
	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX] =
		MAX_FRAGMENTS;

	g_fragments_base = (FRAGMENTS_T *)(g_slot_mem + g_slot_mem_size);
	g_slot_mem_size += frag_mem_size;

	g_free_fragments = g_fragments_base;
	for (i = 0; i < (MAX_FRAGMENTS - 1); i++) {
		*(FRAGMENTS_T **)&g_fragments_base[i] =
			&g_fragments_base[i + 1];
	}
	*(FRAGMENTS_T **)&g_fragments_base[i] = NULL;
	sema_init(&g_free_fragments_sema, MAX_FRAGMENTS);

	if (vchiq_init_state(state, vchiq_slot_zero, 0/*slave*/) !=
		VCHIQ_SUCCESS) {
		err = -EINVAL;
		goto failed_vchiq_init;
	}

	err = request_irq(VCHIQ_DOORBELL_IRQ, vchiq_doorbell_irq,
		IRQF_IRQPOLL, "VCHIQ doorbell",
		state);
	if (err < 0) {
		vchiq_log_error(vchiq_arm_log_level, "%s: failed to register "
			"irq=%d err=%d", __func__,
			VCHIQ_DOORBELL_IRQ, err);
		goto failed_request_irq;
	}

	/* Send the base address of the slots to VideoCore */

	dsb(); /* Ensure all writes have completed */

	bcm_mailbox_write(MBOX_CHAN_VCHIQ, (unsigned int)g_slot_phys);

	vchiq_log_info(vchiq_arm_log_level,
		"vchiq_init - done (slots %x, phys %x)",
		(unsigned int)vchiq_slot_zero, g_slot_phys);

   vchiq_call_connected_callbacks();

   return 0;

failed_request_irq:
failed_vchiq_init:
failed_init_slots:
   dma_free_coherent(NULL, g_slot_mem_size, g_slot_mem, g_slot_phys);

failed_alloc:
   return err;
}

void __exit
vchiq_platform_exit(VCHIQ_STATE_T *state)
{
   free_irq(VCHIQ_DOORBELL_IRQ, state);
   dma_free_coherent(NULL, g_slot_mem_size,
                     g_slot_mem, g_slot_phys);
}


VCHIQ_STATUS_T
vchiq_platform_init_state(VCHIQ_STATE_T *state)
{
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
   state->platform_state = kzalloc(sizeof(VCHIQ_2835_ARM_STATE_T), GFP_KERNEL);
   ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 1;
   status = vchiq_arm_init_state(state, &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state);
   if(status != VCHIQ_SUCCESS)
   {
      ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 0;
   }
   return status;
}

VCHIQ_ARM_STATE_T*
vchiq_platform_get_arm_state(VCHIQ_STATE_T *state)
{
   if(!((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited)
   {
      BUG();
   }
   return &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state;
}

void
remote_event_signal(REMOTE_EVENT_T *event)
{
	wmb();

	event->fired = 1;

	dsb();         /* data barrier operation */

	if (event->armed) {
		/* trigger vc interrupt */

		writel(0, __io_address(ARM_0_BELL2));
	}
}

int
vchiq_copy_from_user(void *dst, const void *src, int size)
{
	if ((uint32_t)src < TASK_SIZE) {
		return copy_from_user(dst, src, size);
	} else {
		memcpy(dst, src, size);
		return 0;
	}
}

VCHIQ_STATUS_T
vchiq_prepare_bulk_data(VCHIQ_BULK_T *bulk, VCHI_MEM_HANDLE_T memhandle,
	void *offset, int size, int dir)
{
	PAGELIST_T *pagelist;
	int ret;

	WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);

	ret = create_pagelist((char __user *)offset, size,
			(dir == VCHIQ_BULK_RECEIVE)
			? PAGELIST_READ
			: PAGELIST_WRITE,
			current,
			&pagelist);
	if (ret != 0)
		return VCHIQ_ERROR;

	bulk->handle = memhandle;
	bulk->data = VCHIQ_ARM_ADDRESS(pagelist);

	/* Store the pagelist address in remote_data, which isn't used by the
	   slave. */
	bulk->remote_data = pagelist;

	return VCHIQ_SUCCESS;
}

void
vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
	if (bulk && bulk->remote_data && bulk->actual)
		free_pagelist((PAGELIST_T *)bulk->remote_data, bulk->actual);
}

void
vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
	/*
	 * This should only be called on the master (VideoCore) side, but
	 * provide an implementation to avoid the need for ifdefery.
	 */
	BUG();
}

void
vchiq_dump_platform_state(void *dump_context)
{
	char buf[80];
	int len;
	len = snprintf(buf, sizeof(buf),
		"  Platform: 2835 (VC master)");
	vchiq_dump(dump_context, buf, len + 1);
}

VCHIQ_STATUS_T
vchiq_platform_suspend(VCHIQ_STATE_T *state)
{
   return VCHIQ_ERROR;
}

VCHIQ_STATUS_T
vchiq_platform_resume(VCHIQ_STATE_T *state)
{
   return VCHIQ_SUCCESS;
}

void
vchiq_platform_paused(VCHIQ_STATE_T *state)
{
}

void
vchiq_platform_resumed(VCHIQ_STATE_T *state)
{
}

int
vchiq_platform_videocore_wanted(VCHIQ_STATE_T* state)
{
   return 1; // autosuspend not supported - videocore always wanted
}

int
vchiq_platform_use_suspend_timer(void)
{
   return 0;
}
void
vchiq_dump_platform_use_state(VCHIQ_STATE_T *state)
{
	vchiq_log_info((vchiq_arm_log_level>=VCHIQ_LOG_INFO),"Suspend timer not in use");
}
void
vchiq_platform_handle_timeout(VCHIQ_STATE_T *state)
{
	(void)state;
}
/*
 * Local functions
 */

static irqreturn_t
vchiq_doorbell_irq(int irq, void *dev_id)
{
	VCHIQ_STATE_T *state = dev_id;
	irqreturn_t ret = IRQ_NONE;
	unsigned int status;

	/* Read (and clear) the doorbell */
	status = readl(__io_address(ARM_0_BELL0));

	if (status & 0x4) {  /* Was the doorbell rung? */
		remote_event_pollall(state);
		ret = IRQ_HANDLED;
	}

	return ret;
}

/* There is a potential problem with partial cache lines (pages?)
** at the ends of the block when reading. If the CPU accessed anything in
** the same line (page?) then it may have pulled old data into the cache,
** obscuring the new data underneath. We can solve this by transferring the
** partial cache lines separately, and allowing the ARM to copy into the
** cached area.

** N.B. This implementation plays slightly fast and loose with the Linux
** driver programming rules, e.g. its use of __virt_to_bus instead of
** dma_map_single, but it isn't a multi-platform driver and it benefits
** from increased speed as a result.
*/

static int
create_pagelist(char __user *buf, size_t count, unsigned short type,
	struct task_struct *task, PAGELIST_T ** ppagelist)
{
	PAGELIST_T *pagelist;
	struct page **pages;
	struct page *page;
	unsigned long *addrs;
	unsigned int num_pages, offset, i;
	char *addr, *base_addr, *next_addr;
	int run, addridx, actual_pages;

	offset = (unsigned int)buf & (PAGE_SIZE - 1);
	num_pages = (count + offset + PAGE_SIZE - 1) / PAGE_SIZE;

	*ppagelist = NULL;

	/* Allocate enough storage to hold the page pointers and the page
	** list
	*/
	pagelist = kmalloc(sizeof(PAGELIST_T) +
		(num_pages * sizeof(unsigned long)) +
		(num_pages * sizeof(pages[0])),
		GFP_KERNEL);

	vchiq_log_trace(vchiq_arm_log_level,
		"create_pagelist - %x", (unsigned int)pagelist);
	if (!pagelist)
		return -ENOMEM;

	addrs = pagelist->addrs;
	pages = (struct page **)(addrs + num_pages);

	down_read(&task->mm->mmap_sem);
	actual_pages = get_user_pages(task, task->mm,
		(unsigned long)buf & ~(PAGE_SIZE - 1), num_pages,
		(type == PAGELIST_READ) /*Write */ , 0 /*Force */ ,
		pages, NULL /*vmas */);
	up_read(&task->mm->mmap_sem);

   if (actual_pages != num_pages)
   {
      /* This is probably due to the process being killed */
      while (actual_pages > 0)
      {
         actual_pages--;
         page_cache_release(pages[actual_pages]);
      }
      kfree(pagelist);
      if (actual_pages == 0)
         actual_pages = -ENOMEM;
      return actual_pages;
   }

	pagelist->length = count;
	pagelist->type = type;
	pagelist->offset = offset;

	/* Group the pages into runs of contiguous pages */

	base_addr = VCHIQ_ARM_ADDRESS(page_address(pages[0]));
	next_addr = base_addr + PAGE_SIZE;
	addridx = 0;
	run = 0;

	for (i = 1; i < num_pages; i++) {
		addr = VCHIQ_ARM_ADDRESS(page_address(pages[i]));
		if ((addr == next_addr) && (run < (PAGE_SIZE - 1))) {
			next_addr += PAGE_SIZE;
			run++;
		} else {
			addrs[addridx] = (unsigned long)base_addr + run;
			addridx++;
			base_addr = addr;
			next_addr = addr + PAGE_SIZE;
			run = 0;
		}
	}

	addrs[addridx] = (unsigned long)base_addr + run;
	addridx++;

	/* Partial cache lines (fragments) require special measures */
	if ((type == PAGELIST_READ) &&
		((pagelist->offset & (CACHE_LINE_SIZE - 1)) ||
		((pagelist->offset + pagelist->length) &
		(CACHE_LINE_SIZE - 1)))) {
		FRAGMENTS_T *fragments;

		if (down_interruptible(&g_free_fragments_sema) != 0) {
			kfree(pagelist);
			return -EINTR;
		}

		WARN_ON(g_free_fragments == NULL);

		down(&g_free_fragments_mutex);
		fragments = (FRAGMENTS_T *) g_free_fragments;
		WARN_ON(fragments == NULL);
		g_free_fragments = *(FRAGMENTS_T **) g_free_fragments;
		up(&g_free_fragments_mutex);
		pagelist->type =
			 PAGELIST_READ_WITH_FRAGMENTS + (fragments -
							 g_fragments_base);
	}

	for (page = virt_to_page(pagelist);
		page <= virt_to_page(addrs + num_pages - 1); page++) {
		flush_dcache_page(page);
	}

	*ppagelist = pagelist;

	return 0;
}

static void
free_pagelist(PAGELIST_T *pagelist, int actual)
{
	struct page **pages;
	unsigned int num_pages, i;

	vchiq_log_trace(vchiq_arm_log_level,
		"free_pagelist - %x, %d", (unsigned int)pagelist, actual);

	num_pages =
		(pagelist->length + pagelist->offset + PAGE_SIZE - 1) /
		PAGE_SIZE;

	pages = (struct page **)(pagelist->addrs + num_pages);

	/* Deal with any partial cache lines (fragments) */
	if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS) {
		FRAGMENTS_T *fragments = g_fragments_base +
			(pagelist->type - PAGELIST_READ_WITH_FRAGMENTS);
		int head_bytes, tail_bytes;
		head_bytes = (CACHE_LINE_SIZE - pagelist->offset) &
			(CACHE_LINE_SIZE - 1);
		tail_bytes = (pagelist->offset + actual) &
			(CACHE_LINE_SIZE - 1);

		if ((actual >= 0) && (head_bytes != 0)) {
			if (head_bytes > actual)
				head_bytes = actual;

			memcpy((char *)page_address(pages[0]) +
				pagelist->offset,
				fragments->headbuf,
				head_bytes);
		}
		if ((actual >= 0) && (head_bytes < actual) &&
			(tail_bytes != 0)) {
			memcpy((char *)page_address(pages[num_pages - 1]) +
				((pagelist->offset + actual) &
				(PAGE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1)),
				fragments->tailbuf, tail_bytes);
		}

		down(&g_free_fragments_mutex);
		*(FRAGMENTS_T **) fragments = g_free_fragments;
		g_free_fragments = fragments;
		up(&g_free_fragments_mutex);
		up(&g_free_fragments_sema);
	}

	for (i = 0; i < num_pages; i++) {
		if (pagelist->type != PAGELIST_WRITE)
			set_page_dirty(pages[i]);
		page_cache_release(pages[i]);
	}

	kfree(pagelist);
}
