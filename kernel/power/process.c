/*
 * drivers/power/process.c - Functions for starting/stopping processes on 
 *                           suspend transitions.
 *
 * Originally from swsusp.
 */


#undef DEBUG

#include <linux/interrupt.h>
#include <linux/oom.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/buffer_head.h>
#include <linux/workqueue.h>

int freezer_state;
EXPORT_SYMBOL_GPL(freezer_state);

int freezer_sync = 1;
EXPORT_SYMBOL_GPL(freezer_sync);

/* 
 * Timeout for stopping processes
 */
#define TIMEOUT	(20 * HZ)

static inline int freezable(struct task_struct * p)
{
	if ((p == current) ||
	    (p->flags & PF_NOFREEZE) ||
	    (p->exit_state != 0))
		return 0;
	return 1;
}

static int try_to_freeze_tasks(bool sig_only)
{
	struct task_struct *g, *p;
	unsigned long end_time;
	unsigned int todo;
	bool wq_busy = false;
	struct timeval start, end;
	u64 elapsed_csecs64;
	unsigned int elapsed_csecs;
	bool wakeup = false;

	do_gettimeofday(&start);

	end_time = jiffies + TIMEOUT;

	if (!sig_only)
		freeze_workqueues_begin();

	while (true) {
		todo = 0;
		read_lock(&tasklist_lock);
		do_each_thread(g, p) {
			if (frozen(p) || !freezable(p))
				continue;

			if (!freeze_task(p, sig_only))
				continue;

			/*
			 * Now that we've done set_freeze_flag, don't
			 * perturb a task in TASK_STOPPED or TASK_TRACED.
			 * It is "frozen enough".  If the task does wake
			 * up, it will immediately call try_to_freeze.
			 *
			 * Because freeze_task() goes through p's
			 * scheduler lock after setting TIF_FREEZE, it's
			 * guaranteed that either we see TASK_RUNNING or
			 * try_to_stop() after schedule() in ptrace/signal
			 * stop sees TIF_FREEZE.
			 */
			if (!task_is_stopped_or_traced(p) &&
			    !freezer_should_skip(p))
				todo++;
		} while_each_thread(g, p);
		read_unlock(&tasklist_lock);

		if (!sig_only) {
			wq_busy = freeze_workqueues_busy();
			todo += wq_busy;
		}

		if (!todo || time_after(jiffies, end_time))
			break;

		if (pm_wakeup_pending()) {
			wakeup = true;
			break;
		}

		/*
		 * We need to retry, but first give the freezing tasks some
		 * time to enter the regrigerator.
		 */
		msleep(10);
	}

	do_gettimeofday(&end);
	elapsed_csecs64 = timeval_to_ns(&end) - timeval_to_ns(&start);
	do_div(elapsed_csecs64, NSEC_PER_SEC / 100);
	elapsed_csecs = elapsed_csecs64;

	if (todo) {
		/* This does not unfreeze processes that are already frozen
		 * (we have slightly ugly calling convention in that respect,
		 * and caller must call thaw_processes() if something fails),
		 * but it cleans up leftover PF_FREEZE requests.
		 */
		printk("\n");
		printk(KERN_ERR "Freezing of tasks %s after %d.%02d seconds "
		       "(%d tasks refusing to freeze, wq_busy=%d):\n",
		       wakeup ? "aborted" : "failed",
		       elapsed_csecs / 100, elapsed_csecs % 100,
		       todo - wq_busy, wq_busy);

		thaw_workqueues();

		read_lock(&tasklist_lock);
		do_each_thread(g, p) {
			task_lock(p);
			if (!wakeup && freezing(p) && !freezer_should_skip(p))
				sched_show_task(p);
			cancel_freezing(p);
			task_unlock(p);
		} while_each_thread(g, p);
		read_unlock(&tasklist_lock);
	} else {
		printk("(elapsed %d.%02d seconds) ", elapsed_csecs / 100,
			elapsed_csecs % 100);
	}

	return todo ? -EBUSY : 0;
}

/**
 *	freeze_processes - tell processes to enter the refrigerator
 */
int freeze_processes(void)
{
	int error;

	printk(KERN_INFO "Stopping fuse filesystems.\n");
	freeze_filesystems(FS_FREEZER_FUSE);
	freezer_state = FREEZER_FILESYSTEMS_FROZEN;
	printk(KERN_INFO "Freezing user space processes ... ");
	error = try_to_freeze_tasks(true);
	if (error)
		goto Exit;
	printk("done.\n");

	if (freezer_sync)
		sys_sync();
	printk(KERN_INFO "Stopping normal filesystems.\n");
	freeze_filesystems(FS_FREEZER_NORMAL);
	freezer_state = FREEZER_USERSPACE_FROZEN;
	printk(KERN_INFO "Freezing remaining freezable tasks ... ");
	error = try_to_freeze_tasks(false);
	if (error)
		goto Exit;
	printk("done.");
	freezer_state = FREEZER_FULLY_ON;

	oom_killer_disable();
 Exit:
	BUG_ON(in_atomic());
	printk("\n");

	return error;
}
EXPORT_SYMBOL_GPL(freeze_processes);

static void thaw_tasks(bool nosig_only)
{
	struct task_struct *g, *p;

	read_lock(&tasklist_lock);
	do_each_thread(g, p) {
		if (!freezable(p))
			continue;

		if (nosig_only && should_send_signal(p))
			continue;

		if (cgroup_freezing_or_frozen(p))
			continue;

		thaw_process(p);
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);
}

void thaw_processes(void)
{
	int old_state = freezer_state;

	if (old_state == FREEZER_OFF)
		return;

	freezer_state = FREEZER_OFF;

	oom_killer_enable();

	printk(KERN_INFO "Restarting all filesystems ...\n");
	thaw_filesystems(FS_FREEZER_ALL);

	printk(KERN_INFO "Restarting tasks ... ");
	if (old_state == FREEZER_FULLY_ON) {
		thaw_workqueues();
		thaw_tasks(true);
	}

	thaw_tasks(false);
	schedule();
	printk("done.\n");
}
EXPORT_SYMBOL_GPL(thaw_processes);

void thaw_kernel_threads(void)
{
	freezer_state = FREEZER_USERSPACE_FROZEN;
	printk(KERN_INFO "Restarting normal filesystems.\n");
	thaw_filesystems(FS_FREEZER_NORMAL);
	thaw_workqueues();
	thaw_tasks(true);
}

/*
 * It's ugly putting this EXPORT down here, but it's necessary so that it
 * doesn't matter whether the fs-freezing patch is applied or not.
 */
EXPORT_SYMBOL_GPL(thaw_kernel_threads);
