/*
 * include/linux/tuxonice.h
 *
 * Copyright (C) 2011 Nigel Cunningham (nigel at tuxonice net)
 *
 * This file is released under the GPLv2.
 */

#ifndef LINUX_TUXONICE_H
#define LINUX_TUXONICE_H

extern struct memory_bitmap *precompressed_map;

#ifdef CONFIG_TOI_ZRAM_SUPPORT
extern void memory_bm_set_bit(struct memory_bitmap *bm, unsigned long pfn);
#define PagePrecompressed(page) (precompressed_map ? \
		memory_bm_test_bit(precompressed_map, page_to_pfn(page)) : 0)
#define SetPagePrecompressed(page) \
	(memory_bm_set_bit(precompressed_map, page_to_pfn(page)))
#define ClearPagePrecompressed(page) \
	(memory_bm_clear_bit(precompressed_map, page_to_pfn(page)))
extern int (*toi_flag_zram_disks) (void);
#else
#define PagePrecompressed(page) (0)
#define toi_flag_zram_disks (0)
#endif
#endif
