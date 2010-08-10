/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#include "iomux.h"
#include "mx53_pins.h"

/*!
 * @file mach-mx53/mx53_evk_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */

static struct mxc_iomux_pin_cfg __initdata mxc_iomux_pins[] = {
	{
	 MX53_PIN_EIM_WAIT, IOMUX_CONFIG_ALT1,
	 },
	{
	 MX53_PIN_EIM_OE, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_EIM_RW, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_EIM_A25, IOMUX_CONFIG_ALT6,
	 },
	{
	 MX53_PIN_EIM_D16, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_EIM_D17, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_EIM_D18, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_EIM_D20, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_EIM_D23, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_EIM_D24, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_EIM_D26, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_EIM_D28, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_EIM_D29, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_EIM_D30, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_EIM_D31, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_NANDF_CS2, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_NANDF_CS3, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_ATA_BUFFER_EN, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH3,
	 },
	{
	 MX53_PIN_ATA_CS_0, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_CS_1, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH3,
	 },
	{
	 MX53_PIN_ATA_DA_1, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_ATA_DATA4, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_ATA_DATA6, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_ATA_DATA12, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_ATA_DATA13, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_ATA_DATA14, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_ATA_DATA15, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_ATA_DIOR, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART2_IPP_UART_RTS_B_SELECT_INPUT,
	 INPUT_CTL_PATH3,
	 },
	{
	 MX53_PIN_ATA_DIOW, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_ATA_DMACK, IOMUX_CONFIG_ALT3,
	 },
	{
	 MX53_PIN_ATA_DMARQ, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_INTRQ, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_KEY_COL0, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_ROW0, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_COL1, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_ROW1, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_COL2, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_ROW2, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT4,
	 },
	{
	 MX53_PIN_KEY_COL4, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_KEY_ROW4, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX53_PIN_CSI0_D4, IOMUX_CONFIG_ALT5,
	 },
	{
	 MX53_PIN_CSI0_D5, IOMUX_CONFIG_ALT5,
	 },
	{
	 MX53_PIN_CSI0_D6, IOMUX_CONFIG_ALT5,
	 },
	{
	 MX53_PIN_CSI0_D7, IOMUX_CONFIG_ALT5,
	 },
	{
	 MX53_PIN_CSI0_D9, IOMUX_CONFIG_ALT5,
	 },
	{ /* UART1 Tx */
	 MX53_PIN_CSI0_D10, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{ /* UART1 Rx */
	 MX53_PIN_CSI0_D11, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	 PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH1,
	},
	{
	 MX53_PIN_GPIO_2, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_3, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_4, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_5, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_6, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_7, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_8, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_10, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_11, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_12, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_13, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_14, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_16, IOMUX_CONFIG_ALT1,
	 },
	{
	 MX53_PIN_GPIO_17, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_18, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX53_PIN_GPIO_19, IOMUX_CONFIG_ALT3,
	 },
	{	/* DI0 display clock */
	 MX53_PIN_DI0_DISP_CLK, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{	/* DI0 data enable */
	 MX53_PIN_DI0_PIN15, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{	/* DI0 HSYNC */
	 MX53_PIN_DI0_PIN2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{	/* DI0 VSYNC */
	 MX53_PIN_DI0_PIN3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT0, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT4, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT5, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT6, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT7, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT8, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT9, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT10, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT11, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT12, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT13, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT14, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT15, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT16, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT17, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT18, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT19, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT20, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT21, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT22, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX53_PIN_DISP0_DAT23, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_NONE | PAD_CTL_ODE_OPENDRAIN_NONE |
	  PAD_CTL_DRV_LOW | PAD_CTL_SRE_SLOW),
	 },
	/* esdhc1 */
	{
	 MX53_PIN_SD1_CMD, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_SD1_CLK, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_SD1_DATA0, IOMUX_CONFIG_ALT0,
	(PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	 {
	 MX53_PIN_SD1_DATA1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	 {
	 MX53_PIN_SD1_DATA2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	 {
	 MX53_PIN_SD1_DATA3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	/* esdhc3 */
	{
	 MX53_PIN_ATA_DATA0, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA1, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA2, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA3, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA8, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA9, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA10, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_DATA11, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_IORDY, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX53_PIN_ATA_RESET_B, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_75k_PU | PAD_CTL_SRE_FAST),
	 },
	{ /* FEC pins */
	 MX53_PIN_FEC_MDIO, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_22K_PU | PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_DRV_HIGH),
	 MUX_IN_FEC_FEC_MDI_SELECT_INPUT,
	 INPUT_CTL_PATH1,
	 },
	{
	 MX53_PIN_FEC_REF_CLK, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE),
	 },
	{
	 MX53_PIN_FEC_RX_ER, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE),
	 },
	{
	 MX53_PIN_FEC_CRS_DV, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE),
	 },
	{
	 MX53_PIN_FEC_RXD1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE),
	 },
	{
	 MX53_PIN_FEC_RXD0, IOMUX_CONFIG_ALT0,
	(PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE),
	 },
	{
	 MX53_PIN_FEC_TX_EN, IOMUX_CONFIG_ALT0,
	 PAD_CTL_DRV_HIGH,
	 },
	{
	 MX53_PIN_FEC_TXD1, IOMUX_CONFIG_ALT0,
	 PAD_CTL_DRV_HIGH,
	 },
	{
	 MX53_PIN_FEC_TXD0, IOMUX_CONFIG_ALT0,
	 PAD_CTL_DRV_HIGH,
	 },
	{
	 MX53_PIN_FEC_MDC, IOMUX_CONFIG_ALT0,
	 PAD_CTL_DRV_HIGH,
	 },
};

static struct mxc_iomux_pin_cfg __initdata mx53_evk_iomux_pins[] = {
	{ /* USB OTG USB_OC */
	 MX53_PIN_EIM_A24, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB OTG USB_PWR */
	 MX53_PIN_EIM_A23, IOMUX_CONFIG_GPIO,
	 },
	{ /* DISPB0_SER_CLK */
	 MX53_PIN_EIM_D21, IOMUX_CONFIG_ALT3,
	 },
	{ /* DI0_PIN1 */
	 MX53_PIN_EIM_D22, IOMUX_CONFIG_ALT3,
	 },
	{ /* SDHC1 SD_CD */
	 MX53_PIN_EIM_DA13, IOMUX_CONFIG_GPIO,
	 },
	{ /* SDHC1 SD_WP */
	 MX53_PIN_EIM_DA14, IOMUX_CONFIG_GPIO,
	 },
	{ /* SDHC3 SD_CD */
	 MX53_PIN_EIM_DA11, IOMUX_CONFIG_GPIO,
	 },
	{ /* SDHC3 SD_WP */
	 MX53_PIN_EIM_DA12, IOMUX_CONFIG_GPIO,
	 },
	{ /* PWM backlight */
	 MX53_PIN_GPIO_1, IOMUX_CONFIG_ALT4,
	 },
	 { /* USB HOST USB_PWR */
	 MX53_PIN_ATA_DA_2, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB HOST USB_RST */
	 MX53_PIN_CSI0_DATA_EN, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB HOST CARD_ON */
	 MX53_PIN_EIM_DA15, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB HOST CARD_RST */
	 MX53_PIN_ATA_DATA7, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB HOST WAN_WAKE */
	 MX53_PIN_EIM_D25, IOMUX_CONFIG_GPIO,
	 },
	{ /* FEC_RST */
	MX53_PIN_ATA_DA_0, IOMUX_CONFIG_ALT1,
	},
	{ /* audio clock out */
	 MX53_PIN_GPIO_0, IOMUX_CONFIG_ALT3,
	 },
};

static struct mxc_iomux_pin_cfg __initdata mx53_arm2_iomux_pins[] = {
	{ /* USB OTG USB_OC */
	 MX53_PIN_EIM_D21, IOMUX_CONFIG_GPIO,
	 },
	{ /* USB OTG USB_PWR */
	 MX53_PIN_EIM_D22, IOMUX_CONFIG_GPIO,
	 },
	{ /* SDHC1 SD_CD */
	 MX53_PIN_GPIO_1, IOMUX_CONFIG_GPIO,
	 },
	{ /* gpio backlight */
	 MX53_PIN_DI0_PIN4, IOMUX_CONFIG_GPIO,
	},
};

static int __initdata enable_w1 = { 0 };
static int __init w1_setup(char *__unused)
{
	enable_w1 = 1;
	return 1;
}

__setup("w1", w1_setup);

void __init mx53_evk_io_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mxc_iomux_pins); i++) {
		mxc_request_iomux(mxc_iomux_pins[i].pin,
				  mxc_iomux_pins[i].mux_mode);
		if (mxc_iomux_pins[i].pad_cfg)
			mxc_iomux_set_pad(mxc_iomux_pins[i].pin,
					  mxc_iomux_pins[i].pad_cfg);
		if (mxc_iomux_pins[i].in_select)
			mxc_iomux_set_input(mxc_iomux_pins[i].in_select,
					    mxc_iomux_pins[i].in_mode);
	}

	if (board_is_mx53_arm2()) {
		/* MX53 ARM2 CPU board */
		pr_info("MX53 ARM2 board \n");
		for (i = 0; i < ARRAY_SIZE(mx53_arm2_iomux_pins); i++) {
			mxc_request_iomux(mx53_arm2_iomux_pins[i].pin,
					  mx53_arm2_iomux_pins[i].mux_mode);
			if (mx53_arm2_iomux_pins[i].pad_cfg)
				mxc_iomux_set_pad(mx53_arm2_iomux_pins[i].pin,
						  mx53_arm2_iomux_pins[i].pad_cfg);
			if (mx53_arm2_iomux_pins[i].in_select)
				mxc_iomux_set_input(mx53_arm2_iomux_pins[i].in_select,
						    mx53_arm2_iomux_pins[i].in_mode);
		}

		/* Enable OTG VBus with GPIO low */
		mxc_iomux_set_pad(MX53_PIN_EIM_D22, PAD_CTL_DRV_HIGH |
				PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_D22), "gpio3_22");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_EIM_D22), 0);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_EIM_D22), 0);

		gpio_request(IOMUX_TO_GPIO(MX53_PIN_GPIO_1), "gpio1_1");
		gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_GPIO_1));	/* SD1 CD */

		gpio_request(IOMUX_TO_GPIO(MX53_PIN_DI0_PIN4), "gpio4_20");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_DI0_PIN4), 0);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_DI0_PIN4), 1);
	} else {
		/* MX53 EVK board */
		pr_info("MX53 EVK board \n");
			for (i = 0; i < ARRAY_SIZE(mx53_evk_iomux_pins); i++) {
			mxc_request_iomux(mx53_evk_iomux_pins[i].pin,
					  mx53_evk_iomux_pins[i].mux_mode);
			if (mx53_evk_iomux_pins[i].pad_cfg)
				mxc_iomux_set_pad(mx53_evk_iomux_pins[i].pin,
						  mx53_evk_iomux_pins[i].pad_cfg);
			if (mx53_evk_iomux_pins[i].in_select)
				mxc_iomux_set_input(mx53_evk_iomux_pins[i].in_select,
						    mx53_evk_iomux_pins[i].in_mode);
		}
		/* Host1 Vbus with GPIO high */
		mxc_iomux_set_pad(MX53_PIN_ATA_DA_2, PAD_CTL_DRV_HIGH |
				PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_2), "gpio7_8");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_2), 1);

		/* USB HUB RESET - De-assert USB HUB RESET_N */
		mxc_iomux_set_pad(MX53_PIN_CSI0_DATA_EN, PAD_CTL_DRV_HIGH |
				  PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_CSI0_DATA_EN), "gpio5_20");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_CSI0_DATA_EN), 0);

		msleep(1);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_CSI0_DATA_EN), 0);
		msleep(1);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_CSI0_DATA_EN), 1);

		/* Enable OTG VBus with GPIO low */
		mxc_iomux_set_pad(MX53_PIN_EIM_A23, PAD_CTL_DRV_HIGH |
				PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST);
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_A23), "gpio6_6");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_EIM_A23), 0);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_EIM_A23), 0);

		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_DA13), "gpio3_13");
		gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_EIM_DA13));	/* SD1 CD */
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_DA14), "gpio3_14");
		gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_EIM_DA14));	/* SD1 WP */

		/* SD3 CD */
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_DA11), "gpio3_11");
		gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_EIM_DA11));

		/* SD3 WP */
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_EIM_DA12), "gpio3_12");
		gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_EIM_DA11));

		/* reset FEC PHY */
		gpio_request(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_0), "gpio7_6");
		gpio_direction_output(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_0), 0);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_0), 0);
		msleep(1);
		gpio_set_value(IOMUX_TO_GPIO(MX53_PIN_ATA_DA_0), 1);
	}


	gpio_request(IOMUX_TO_GPIO(MX53_PIN_GPIO_16), "gpio7_11");
	gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_GPIO_16));	/*PMIC_INT*/


	/* i2c1 SDA */
	mxc_request_iomux(MX53_PIN_CSI0_D8,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
	mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
			    INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D8, PAD_CTL_SRE_FAST |
			  PAD_CTL_ODE_OPENDRAIN_ENABLE |
			  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
			  PAD_CTL_HYS_ENABLE);

	/* i2c1 SCL */
	mxc_request_iomux(MX53_PIN_CSI0_D9,
			  IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
	mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
			    INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D9, PAD_CTL_SRE_FAST |
			  PAD_CTL_ODE_OPENDRAIN_ENABLE |
			  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
			  PAD_CTL_HYS_ENABLE);

	/* i2c2 SDA */
	mxc_request_iomux(MX53_PIN_KEY_ROW3,
			  IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
	mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
			    INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
			  PAD_CTL_SRE_FAST |
			  PAD_CTL_ODE_OPENDRAIN_ENABLE |
			  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
			  PAD_CTL_HYS_ENABLE);

	/* i2c2 SCL */
	mxc_request_iomux(MX53_PIN_KEY_COL3,
			  IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
	mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
			    INPUT_CTL_PATH0);
	mxc_iomux_set_pad(MX53_PIN_KEY_COL3,
			  PAD_CTL_SRE_FAST |
			  PAD_CTL_ODE_OPENDRAIN_ENABLE |
			  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
			  PAD_CTL_HYS_ENABLE);

	/* headphone_det_b */
	mxc_request_iomux(MX53_PIN_ATA_DATA5, IOMUX_CONFIG_GPIO);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA5, PAD_CTL_100K_PU);
	gpio_request(IOMUX_TO_GPIO(MX53_PIN_ATA_DATA5), "gpio2_5");
	gpio_direction_input(IOMUX_TO_GPIO(MX53_PIN_ATA_DATA5));

	/* power key */

	/* LCD related gpio */

	/* Camera reset */

	/* Camera low power */

}

/* workaround for ecspi chipselect pin may not keep correct level when idle */
void mx53_evk_gpio_spi_chipselect_active(int cspi_mode, int status,
					     int chipselect)
{
	u32 gpio;

	switch (cspi_mode) {
	case 1:
		switch (chipselect) {
		case 0x1:
			mxc_request_iomux(MX53_PIN_EIM_D19,
					  IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_EIM_D19,
					  PAD_CTL_HYS_ENABLE |
					  PAD_CTL_PKE_ENABLE |
					  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
			break;
		case 0x2:
			gpio = IOMUX_TO_GPIO(MX53_PIN_EIM_D19);
			mxc_request_iomux(MX53_PIN_EIM_D19,
					  IOMUX_CONFIG_GPIO);
			gpio_request(gpio, "cspi1_ss1");
			gpio_direction_output(gpio, 0);
			gpio_set_value(gpio, 1 & (~status));
			break;
		default:
			break;
		}
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(mx53_evk_gpio_spi_chipselect_active);

void mx53_evk_gpio_spi_chipselect_inactive(int cspi_mode, int status,
					       int chipselect)
{
	switch (cspi_mode) {
	case 1:
		switch (chipselect) {
		case 0x1:
			mxc_free_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_EIM_D19,
					  IOMUX_CONFIG_GPIO);
			mxc_free_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_GPIO);
			break;
		case 0x2:
			mxc_free_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_GPIO);
			break;
		default:
			break;
		}
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(mx53_evk_gpio_spi_chipselect_inactive);

void gpio_lcd_active(void)
{
/* TO DO */
}
EXPORT_SYMBOL(gpio_lcd_active);
