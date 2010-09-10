/*
 * Compressed RAM based swap device
 *
 * Copyright (C) 2008, 2009  Nitin Gupta
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 *
 * Project home: http://compcache.googlecode.com
 */

#ifndef _RAMZSWAP_DRV_H_
#define _RAMZSWAP_DRV_H_

#include "ramzswap_ioctl.h"
#include "xvmalloc.h"

/*
 * Some arbitrary value. This is just to catch
 * invalid value for num_devices module parameter.
 */
static const unsigned max_num_devices = 32;

/*
 * Stored at beginning of each compressed object.
 *
 * It stores back-reference to table entry which points to this
 * object. This is required to support memory defragmentation or
 * migrating compressed pages to backing swap disk.
 */
struct zobj_header {
#if 0
	u32 table_idx;
#endif
};

/*-- Configurable parameters */

/* Default ramzswap disk size: 25% of total RAM */
static const unsigned default_disksize_perc_ram = 25;
static const unsigned default_memlimit_perc_ram = 15;

/*
 * Max compressed page size when backing device is provided.
 * Pages that compress to size greater than this are sent to
 * physical swap disk.
 */
static const unsigned max_zpage_size_bdev = PAGE_SIZE / 2;

/*
 * Max compressed page size when there is no backing dev.
 * Pages that compress to size greater than this are stored
 * uncompressed in memory.
 */
static const unsigned max_zpage_size_nobdev = PAGE_SIZE / 4 * 3;

/*
 * NOTE: max_zpage_size_{bdev,nobdev} sizes must be
 * less than or equal to:
 *   XV_MAX_ALLOC_SIZE - sizeof(struct zobj_header)
 * since otherwise xv_malloc would always return failure.
 */

/*-- End of configurable params */

#define SECTOR_SHIFT		9
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)

/* Debugging and Stats */
#if defined(CONFIG_RAMZSWAP_STATS)
#define stat_inc(stat)	((stat)++)
#define stat_dec(stat)	((stat)--)
#else
#define stat_inc(x)
#define stat_dec(x)
#endif

/* Flags for ramzswap pages (table[page_no].flags) */
enum rzs_pageflags {
	/* Page is stored uncompressed */
	RZS_UNCOMPRESSED,

	/* Page consists entirely of zeros */
	RZS_ZERO,

	__NR_RZS_PAGEFLAGS,
};

/*-- Data structures */

/*
 * Allocated for each swap slot, indexed by page no.
 * These table entries must fit exactly in a page.
 */
struct table {
	struct page *page;
	u16 offset;
	u8 count;	/* object ref count (not yet used) */
	u8 flags;
} __attribute__((aligned(4)));;

/*
 * Swap extent information in case backing swap is a regular
 * file. These extent entries must fit exactly in a page.
 */
struct ramzswap_backing_extent {
	pgoff_t phy_pagenum;
	pgoff_t num_pages;
} __attribute__((aligned(4)));

struct ramzswap_stats {
	/* basic stats */
	size_t compr_size;	/* compressed size of pages stored -
				 * needed to enforce memlimit */
	/* more stats */
#if defined(CONFIG_RAMZSWAP_STATS)
	u64 num_reads;		/* failed + successful */
	u64 num_writes;		/* --do-- */
	u64 failed_reads;	/* can happen when memory is too low */
	u64 failed_writes;	/* should NEVER! happen */
	u64 invalid_io;		/* non-swap I/O requests */
	u32 pages_zero;		/* no. of zero filled pages */
	u32 pages_stored;	/* no. of pages currently stored */
	u32 good_compress;	/* % of pages with compression ratio<=50% */
	u32 pages_expand;	/* % of incompressible pages */
	u64 bdev_num_reads;	/* no. of reads on backing dev */
	u64 bdev_num_writes;	/* no. of writes on backing dev */
#endif
};

struct ramzswap {
	struct xv_pool *mem_pool;
	void *compress_workmem;
	void *compress_buffer;
	struct table *table;
	struct mutex lock;
	struct request_queue *queue;
	struct gendisk *disk;
	int init_done;
	/*
	 * This is limit on compressed data size (stats.compr_size)
	 * Its applicable only when backing swap device is present.
	 */
	size_t memlimit;	/* bytes */
	/*
	 * This is limit on amount of *uncompressed* worth of data
	 * we can hold. When backing swap device is provided, it is
	 * set equal to device size.
	 */
	size_t disksize;	/* bytes */

	struct ramzswap_stats stats;

	/* backing swap device info */
	struct ramzswap_backing_extent *curr_extent;
	struct list_head backing_swap_extent_list;
	unsigned long num_extents;
	char backing_swap_name[MAX_SWAP_NAME_LEN];
	struct block_device *backing_swap;
	struct file *swap_file;
};

/*-- */

#endif

