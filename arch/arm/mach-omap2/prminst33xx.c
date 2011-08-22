/*
 * AM33XX PRM instance functions
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/io.h>

#include <plat/common.h>

#include "prm33xx.h"
#include "prminst33xx.h"
#include "prm-regbits-33xx.h"

#define AM33XX_PRM_MOD_SIZE	0x100
#define AM33XX_PRM_MOD_START	AM33XX_PRM_PER_MOD
#define PRM_REG_SZ	0x4

static u16 off_fixup[][2] = {
	{ 0xC, 0x8 },	/* AM33XX_PRM_PER_MOD */
	{ 0x4, 0x8 },	/* AM33XX_PRM_WKUP_MOD */
	{ 0x0, 0x4 },	/* AM33XX_PRM_MPU_MOD */
	/* XXX: PRM_DEVICE: offsets are invalid for powerdomain*/
	{ 0x0, 0x0 },	/* AM33XX_PRM_DEVICE_MOD */
	{ 0x0, 0x4 },	/* AM33XX_PRM_RTC_MOD */
	{ 0x0, 0x10 },	/* AM33XX_PRM_GFX_MOD */
	{ 0x0, 0x4 },	/* AM33XX_PRM_CEFUSE_MOD */
};

/* Read a register in a PRM instance */
u32 am33xx_prminst_read_inst_reg(s16 inst, u16 idx)
{
	int i = (inst - AM33XX_PRM_MOD_START) / AM33XX_PRM_MOD_SIZE;

	return __raw_readl(prm_base + inst + off_fixup[i][idx / PRM_REG_SZ]);
}

/* Write into a register in a PRM instance */
void am33xx_prminst_write_inst_reg(u32 val, s16 inst, u16 idx)
{
	int i = (inst - AM33XX_PRM_MOD_START) / AM33XX_PRM_MOD_SIZE;

	__raw_writel(val, prm_base + inst + off_fixup[i][idx / PRM_REG_SZ]);
}

/* Read-modify-write a register in PRM. Caller must lock */
u32 am33xx_prminst_rmw_inst_reg_bits(u32 mask, u32 bits, s16 inst,
				   s16 idx)
{
	u32 v;

	v = am33xx_prminst_read_inst_reg(inst, idx);
	v &= ~mask;
	v |= bits;
	am33xx_prminst_write_inst_reg(v, inst, idx);

	return v;
}
