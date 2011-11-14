/*
 * This file contains the address info for various AM33XX modules.
 *
 * Copyright (C) 2011 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_AM33XX_H
#define __ASM_ARCH_AM33XX_H

#define L4_SLOW_AM33XX_BASE	0x48000000

#define AM33XX_SCM_BASE		0x44E10000
#define AM33XX_CTRL_BASE	AM33XX_SCM_BASE
#define AM33XX_PRCM_BASE	0x44E00000

#define AM33XX_GPIO0_BASE	0x44E07000
#define AM33XX_GPIO1_BASE	0x4804C000
#define AM33XX_GPIO2_BASE	0x481AC000
#define AM33XX_GPIO3_BASE	0x481AE000

#define AM33XX_TSC_BASE		0x44E0D000
#define AM33XX_RTC_BASE		0x44E3E000

#define AM33XX_D_CAN0_BASE	0x481CC000
#define AM33XX_D_CAN1_BASE	0x481D0000

#define AM33XX_ASP0_BASE	0x48038000
#define AM33XX_ASP1_BASE	0x4803C000

#define AM33XX_MMC0_BASE	0x48060100
#define AM33XX_MMC1_BASE	0x481D8100
#define AM33XX_MMC2_BASE	0x47810100

#define AM33XX_SPI0_BASE	0x48030000
#define AM33XX_SPI1_BASE	0x481A0000
#endif /* __ASM_ARCH_AM33XX_H */
