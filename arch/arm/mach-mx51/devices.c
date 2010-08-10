/*
 * Copyright 2008-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uio_driver.h>
#include <linux/mxc_scc2_driver.h>
#include <linux/iram_alloc.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/sdma.h>
#include "sdma_script_code.h"
#include "crm_regs.h"

/* Flag used to indicate when IRAM has been initialized */
int iram_ready;
/* Flag used to indicate if dvfs_core is active. */
int dvfs_core_is_active;

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
	/* AP<->BP */
	sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = -1;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = -1;
	sdma_script_addr->mxc_sdma_ap_2_ap_fixed_addr = -1;

	/*misc */
	sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;

	/* firi */
	sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_firi_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_firi_addr = -1;

	/* uart */
	sdma_script_addr->mxc_sdma_uart_2_per_addr = uart_2_per_ADDR;
	sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_ADDR;

	/* UART SH */
	sdma_script_addr->mxc_sdma_uartsh_2_per_addr = uartsh_2_per_ADDR;
	sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr = uartsh_2_mcu_ADDR;

	/* SHP */
	sdma_script_addr->mxc_sdma_per_2_shp_addr = per_2_shp_ADDR;
	sdma_script_addr->mxc_sdma_shp_2_per_addr = shp_2_per_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_ADDR;
	sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_ADDR;

	/* ATA */
	sdma_script_addr->mxc_sdma_mcu_2_ata_addr = mcu_2_ata_ADDR;
	sdma_script_addr->mxc_sdma_ata_2_mcu_addr = ata_2_mcu_ADDR;

	/* app */
	sdma_script_addr->mxc_sdma_app_2_per_addr = app_2_per_ADDR;
	sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_per_2_app_addr = per_2_app_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_ADDR;

	/* MSHC */
	sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = -1;

	/* spdif */
	sdma_script_addr->mxc_sdma_spdif_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_spdif_addr = mcu_2_spdif_ADDR;

	/* IPU */
	sdma_script_addr->mxc_sdma_ext_mem_2_ipu_addr = ext_mem__ipu_ram_ADDR;

	/* DVFS */
	sdma_script_addr->mxc_sdma_dptc_dvfs_addr = -1;

	/* core */
	sdma_script_addr->mxc_sdma_start_addr = (unsigned short *)sdma_code;
	sdma_script_addr->mxc_sdma_ram_code_start_addr = RAM_CODE_START_ADDR;
	sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
}

static struct resource mxc_w1_master_resources[] = {
	{
		.start = OWIRE_BASE_ADDR,
		.end   = OWIRE_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_OWIRE,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_w1_master_device = {
	.name = "mxc_w1",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_w1_master_resources),
	.resource = mxc_w1_master_resources,
};

static struct resource mxc_kpp_resources[] = {
	{
		.start = KPP_BASE_ADDR,
		.end = KPP_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_KPP,
		.end = MXC_INT_KPP,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_kpp_resources),
	.resource = mxc_kpp_resources,
};

static struct resource rtc_resources[] = {
	{
		.start = SRTC_BASE_ADDR,
		.end = SRTC_BASE_ADDR + 0x40,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_SRTC_NTZ,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_rtc_device = {
	.name = "mxc_rtc",
	.id = 0,
	.num_resources = ARRAY_SIZE(rtc_resources),
	.resource = rtc_resources,
};

struct platform_device mxc_nandv2_mtd_device = {
	.name = "mxc_nandv2_flash",
	.id = 0,
};

static struct resource imx_nfc_resources[] = {
	{
		.flags = IORESOURCE_MEM,
		.start = NFC_BASE_ADDR_AXI + 0x0000,
		.end   = NFC_BASE_ADDR_AXI + 0x1200 - 1,
		.name  = IMX_NFC_BUFFERS_ADDR_RES_NAME,
	},
	{
		.flags = IORESOURCE_MEM,
		.start = NFC_BASE_ADDR_AXI + 0x1E00,
		.end   = NFC_BASE_ADDR_AXI + 0x1E44 - 1,
		.name  = IMX_NFC_PRIMARY_REGS_ADDR_RES_NAME,
	},
	{
		.flags = IORESOURCE_MEM,
		.start = NFC_BASE_ADDR + 0x00,
		.end   = NFC_BASE_ADDR + 0x34 - 1,
		.name  = IMX_NFC_SECONDARY_REGS_ADDR_RES_NAME,
	},
	{
		.flags = IORESOURCE_IRQ,
		.start = MXC_INT_NFC,
		.end   = MXC_INT_NFC,
		.name  = IMX_NFC_INTERRUPT_RES_NAME,
	},
};

struct platform_device imx_nfc_device = {
	.name = IMX_NFC_DRIVER_NAME,
	.id = 0,
	.resource      = imx_nfc_resources,
	.num_resources = ARRAY_SIZE(imx_nfc_resources),
};

static struct resource wdt_resources[] = {
	{
		.start = WDOG1_BASE_ADDR,
		.end = WDOG1_BASE_ADDR + 0x30,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mxc_wdt_device = {
	.name = "mxc_wdt",
	.id = 0,
	.num_resources = ARRAY_SIZE(wdt_resources),
	.resource = wdt_resources,
};

static struct resource pwm1_resources[] = {
	{
		.start = PWM1_BASE_ADDR,
		.end = PWM1_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_PWM1,
		.end = MXC_INT_PWM1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_pwm1_device = {
	.name = "mxc_pwm",
	.id = 0,
	.num_resources = ARRAY_SIZE(pwm1_resources),
	.resource = pwm1_resources,
};

static struct resource pwm2_resources[] = {
	{
		.start = PWM2_BASE_ADDR,
		.end = PWM2_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_PWM2,
		.end = MXC_INT_PWM2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_pwm2_device = {
	.name = "mxc_pwm",
	.id = 1,
	.num_resources = ARRAY_SIZE(pwm2_resources),
	.resource = pwm2_resources,
};

struct platform_device mxc_pwm_backlight_device = {
	.name = "pwm-backlight",
	.id = -1,
};

static struct resource ipu_resources[] = {
	{
		.start = IPU_CTRL_BASE_ADDR,
		.end = IPU_CTRL_BASE_ADDR + SZ_512M,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_IPU_SYN,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MXC_INT_IPU_ERR,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_ipu_device = {
	.name = "mxc_ipu",
	.id = -1,
	.num_resources = ARRAY_SIZE(ipu_resources),
	.resource = ipu_resources,
};

struct platform_device mxc_fb_devices[] = {
	{
		.name = "mxc_sdc_fb",
		.id = 0,
		.dev = {
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	},
	{
		.name = "mxc_sdc_fb",
		.id = 1,
		.dev = {
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	},
	{
		.name = "mxc_sdc_fb",
		.id = 2,
		.dev = {
			.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	},
};

static struct resource vpu_resources[] = {
	{
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IO_ADDRESS(SRC_BASE_ADDR),
		.end = IO_ADDRESS(SRC_BASE_ADDR),
		.flags = IORESOURCE_MEM,
	},
	{
		.start	= MXC_INT_VPU_IDLE,
		.end	= MXC_INT_VPU_IDLE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device mxcvpu_device = {
	.name = "mxc_vpu",
	.id = 0,
	.num_resources = ARRAY_SIZE(vpu_resources),
	.resource = vpu_resources,
};

static struct resource mxc_fec_resources[] = {
	{
		.start	= FEC_BASE_ADDR,
		.end	= FEC_BASE_ADDR + SZ_4K - 1,
		.flags	= IORESOURCE_MEM
	},
	{
		.start	= MXC_INT_FEC,
		.end	= MXC_INT_FEC,
		.flags	= IORESOURCE_IRQ
	},
};

struct platform_device mxc_fec_device = {
	.name = "fec",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_fec_resources),
	.resource = mxc_fec_resources,
};

static struct resource mxcspi1_resources[] = {
	{
		.start = CSPI1_BASE_ADDR,
		.end = CSPI1_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_CSPI1,
		.end = MXC_INT_CSPI1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxcspi1_device = {
	.name = "mxc_spi",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxcspi1_resources),
	.resource = mxcspi1_resources,
};

static struct resource mxcspi2_resources[] = {
	{
		.start = CSPI2_BASE_ADDR,
		.end = CSPI2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_CSPI2,
		.end = MXC_INT_CSPI2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxcspi2_device = {
	.name = "mxc_spi",
	.id = 1,
	.num_resources = ARRAY_SIZE(mxcspi2_resources),
	.resource = mxcspi2_resources,
};

static struct resource mxcspi3_resources[] = {
	{
		.start = CSPI3_BASE_ADDR,
		.end = CSPI3_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_CSPI,
		.end = MXC_INT_CSPI,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxcspi3_device = {
	.name = "mxc_spi",
	.id = 2,
	.num_resources = ARRAY_SIZE(mxcspi3_resources),
	.resource = mxcspi3_resources,
};

static struct resource mxci2c1_resources[] = {
	{
		.start = I2C1_BASE_ADDR,
		.end = I2C1_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_I2C1,
		.end = MXC_INT_I2C1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource mxci2c2_resources[] = {
	{
		.start = I2C2_BASE_ADDR,
		.end = I2C2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_I2C2,
		.end = MXC_INT_I2C2,
		.flags = IORESOURCE_IRQ,
	},
};


struct platform_device mxci2c_devices[] = {
	{
		.name = "mxc_i2c",
		.id = 0,
		.num_resources = ARRAY_SIZE(mxci2c1_resources),
		.resource = mxci2c1_resources,
	},
	{
		.name = "mxc_i2c",
		.id = 1,
		.num_resources = ARRAY_SIZE(mxci2c2_resources),
		.resource = mxci2c2_resources,
	},
};

static struct resource mxci2c_hs_resources[] = {
	{
		.start = HSI2C_DMA_BASE_ADDR,
		.end = HSI2C_DMA_BASE_ADDR + SZ_16K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_HS_I2C,
		.end = MXC_INT_HS_I2C,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxci2c_hs_device = {
	.name = "mxc_i2c_hs",
	.id = 3,
	.num_resources = ARRAY_SIZE(mxci2c_hs_resources),
	.resource = mxci2c_hs_resources
};

static struct resource ssi1_resources[] = {
	{
		.start = SSI1_BASE_ADDR,
		.end = SSI1_BASE_ADDR + 0x5C,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_SSI1,
		.end = MXC_INT_SSI1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_ssi1_device = {
	.name = "mxc_ssi",
	.id = 0,
	.num_resources = ARRAY_SIZE(ssi1_resources),
	.resource = ssi1_resources,
};

static struct resource ssi2_resources[] = {
	{
		.start = SSI2_BASE_ADDR,
		.end = SSI2_BASE_ADDR + 0x5C,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_SSI2,
		.end = MXC_INT_SSI2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_ssi2_device = {
	.name = "mxc_ssi",
	.id = 1,
	.num_resources = ARRAY_SIZE(ssi2_resources),
	.resource = ssi2_resources,
};

static struct resource tve_resources[] = {
	{
		.start = TVE_BASE_ADDR,
		.end = TVE_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_TVE,
		.end = MXC_INT_TVE,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_tve_device = {
	.name = "tve",
	.num_resources = ARRAY_SIZE(tve_resources),
	.resource = tve_resources,
};

static struct resource dvfs_core_resources[] = {
	{
		.start = MXC_DVFS_CORE_BASE,
		.end = MXC_DVFS_CORE_BASE + 4 * SZ_16 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_GPC1,
		.end = MXC_INT_GPC1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_dvfs_core_device = {
	.name = "mxc_dvfs_core",
	.id = 0,
	.num_resources = ARRAY_SIZE(dvfs_core_resources),
	.resource = dvfs_core_resources,
};

static struct resource dvfs_per_resources[] = {
	{
		.start = DVFSPER_BASE_ADDR,
		.end = DVFSPER_BASE_ADDR + 2 * SZ_16 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_GPC1,
		.end = MXC_INT_GPC1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_dvfs_per_device = {
	 .name = "mxc_dvfsper",
	 .id = 0,
	 .num_resources = ARRAY_SIZE(dvfs_per_resources),
	 .resource = dvfs_per_resources,
};

struct mxc_gpio_port mxc_gpio_ports[] = {
	{
		.chip.label = "gpio-0",
		.base = IO_ADDRESS(GPIO1_BASE_ADDR),
		.irq = MXC_INT_GPIO1_LOW,
		.irq_high = MXC_INT_GPIO1_HIGH,
		.virtual_irq_start = MXC_GPIO_IRQ_START
	},
	{
		.chip.label = "gpio-1",
		.base = IO_ADDRESS(GPIO2_BASE_ADDR),
		.irq = MXC_INT_GPIO2_LOW,
		.irq_high = MXC_INT_GPIO2_HIGH,
		.virtual_irq_start = MXC_GPIO_IRQ_START + 32 * 1
	},
	{
		.chip.label = "gpio-2",
		.base = IO_ADDRESS(GPIO3_BASE_ADDR),
		.irq = MXC_INT_GPIO3_LOW,
		.irq_high = MXC_INT_GPIO3_HIGH,
		.virtual_irq_start = MXC_GPIO_IRQ_START + 32 * 2
	},
	{
		.chip.label = "gpio-3",
		.base = IO_ADDRESS(GPIO4_BASE_ADDR),
		.irq = MXC_INT_GPIO4_LOW,
		.irq_high = MXC_INT_GPIO4_HIGH,
		.virtual_irq_start = MXC_GPIO_IRQ_START + 32 * 3
	},
};

int __init mxc_register_gpios(void)
{
	return mxc_gpio_init(mxc_gpio_ports, ARRAY_SIZE(mxc_gpio_ports));
}

static struct resource spdif_resources[] = {
	{
		.start = SPDIF_BASE_ADDR,
		.end = SPDIF_BASE_ADDR + 0x50,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_SPDIF,
		.end = MXC_INT_SPDIF,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_alsa_spdif_device = {
	.name = "mxc_alsa_spdif",
	.id = 0,
	.num_resources = ARRAY_SIZE(spdif_resources),
	.resource = spdif_resources,
};

struct platform_device mx51_lpmode_device = {
	.name = "mx51_lpmode",
	.id = 0,
};

struct platform_device busfreq_device = {
	.name = "busfreq",
	.id = 0,
};

static struct resource mxc_m4if_resources[] = {
	{
		.start = M4IF_BASE_ADDR,
		.end = M4IF_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device sdram_autogating_device = {
	.name = "sdram_autogating",
	.id = 0,
	.resource = mxc_m4if_resources,
	.num_resources = ARRAY_SIZE(mxc_m4if_resources),
};

static struct resource mxc_iim_resources[] = {
	{
		.start = IIM_BASE_ADDR,
		.end = IIM_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mxc_iim_device = {
	.name = "mxc_iim",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_iim_resources),
	.resource = mxc_iim_resources
};

static struct resource mxc_sim_resources[] = {
	{
		.start = SIM_BASE_ADDR,
		.end = SIM_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_SIM_IPB,
		.end = MXC_INT_SIM_IPB,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MXC_INT_SIM_DAT,
		.end = MXC_INT_SIM_DAT,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_sim_device = {
	.name = "mxc_sim",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_sim_resources),
	.resource = mxc_sim_resources,
};

static struct resource mxcsdhc1_resources[] = {
	{
		.start = MMC_SDHC1_BASE_ADDR,
		.end = MMC_SDHC1_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_MMC_SDHC1,
		.end = MXC_INT_MMC_SDHC1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource mxcsdhc2_resources[] = {
	{
		.start = MMC_SDHC2_BASE_ADDR,
		.end = MMC_SDHC2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_MMC_SDHC2,
		.end = MXC_INT_MMC_SDHC2,
		.flags = IORESOURCE_IRQ,
	},
	{
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxcsdhc1_device = {
	.name = "mxsdhci",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxcsdhc1_resources),
	.resource = mxcsdhc1_resources,
};

struct platform_device mxcsdhc2_device = {
	.name = "mxsdhci",
	.id = 1,
	.num_resources = ARRAY_SIZE(mxcsdhc2_resources),
	.resource = mxcsdhc2_resources,
};

static struct resource pata_fsl_resources[] = {
	{
		.start = ATA_BASE_ADDR,
		.end = ATA_BASE_ADDR + 0x000000C8,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_ATA,
		.end = MXC_INT_ATA,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device pata_fsl_device = {
	.name = "pata_fsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(pata_fsl_resources),
	.resource = pata_fsl_resources,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static u64 usb_dma_mask = DMA_BIT_MASK(32);

static struct resource usbotg_resources[] = {
	{
		.start = OTG_BASE_ADDR,
		.end = OTG_BASE_ADDR + 0x1ff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_USB_OTG,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource usbotg_xcvr_resources[] = {
	{
		.start = OTG_BASE_ADDR,
		.end = OTG_BASE_ADDR + 0x1ff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_USB_OTG,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_usbdr_udc_device = {
	.name = "fsl-usb2-udc",
	.id   = -1,
	.dev  = {
		.dma_mask = &usb_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.resource      = usbotg_resources,
	.num_resources = ARRAY_SIZE(usbotg_resources),
};

struct platform_device mxc_usbdr_otg_device = {
	.name = "fsl-usb2-otg",
	.id = -1,
	.dev = {
		.dma_mask = &usb_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.resource      = usbotg_xcvr_resources,
	.num_resources = ARRAY_SIZE(usbotg_xcvr_resources),
};

struct platform_device mxc_usbdr_host_device = {
	.name = "fsl-ehci",
	.id = 0,
	.num_resources = ARRAY_SIZE(usbotg_resources),
	.resource = usbotg_resources,
	.dev = {
		.dma_mask = &usb_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct resource usbh1_resources[] = {
	{
		.start = OTG_BASE_ADDR + 0x200,
		.end = OTG_BASE_ADDR + 0x200 + 0x1ff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_USB_H1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_usbh1_device = {
	.name = "fsl-ehci",
	.id = 1,
	.num_resources = ARRAY_SIZE(usbh1_resources),
	.resource = usbh1_resources,
	.dev = {
		.dma_mask = &usb_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct resource usbh2_resources[] = {
	{
		.start = OTG_BASE_ADDR + 0x400,
		.end = OTG_BASE_ADDR + 0x400 + 0x1ff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_USB_H2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mxc_usbh2_device = {
	.name = "fsl-ehci",
	.id = 2,
	.num_resources = ARRAY_SIZE(usbh2_resources),
	.resource = usbh2_resources,
	.dev = {
		.dma_mask = &usb_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct resource mxc_gpu_resources[] = {
	{
		.start = MXC_INT_GPU2_IRQ,
		.end = MXC_INT_GPU2_IRQ,
		.name = "gpu_2d_irq",
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = MXC_INT_GPU,
		.end = MXC_INT_GPU,
		.name = "gpu_3d_irq",
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = GPU2D_BASE_ADDR,
		.end = GPU2D_BASE_ADDR + SZ_4K - 1,
		.name = "gpu_2d_registers",
		.flags = IORESOURCE_MEM,
	},
	{
		.start = GPU_BASE_ADDR,
		.end = GPU_BASE_ADDR + SZ_128K - 1,
		.name = "gpu_3d_registers",
		.flags = IORESOURCE_MEM,
	},
	{
		.start = GPU_GMEM_BASE_ADDR,
		.end = GPU_GMEM_BASE_ADDR + SZ_128K - 1,
		.name = "gpu_graphics_mem",
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device gpu_device = {
	.name = "mxc_gpu",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_gpu_resources),
	.resource = mxc_gpu_resources,
};

static struct resource mxc_gpu2d_resources[] = {
	{
		.start = GPU2D_BASE_ADDR,
		.end = GPU2D_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.flags = IORESOURCE_MEM,
	},
	{
		.flags = IORESOURCE_MEM,
	},
};

#if defined(CONFIG_UIO_PDRV_GENIRQ) || defined(CONFIG_UIO_PDRV_GENIRQ_MODULE)
static struct clk *gpu_clk;

int gpu2d_open(struct uio_info *info, struct inode *inode)
{
	gpu_clk = clk_get(NULL, "gpu2d_clk");
	if (IS_ERR(gpu_clk))
		return PTR_ERR(gpu_clk);

	return clk_enable(gpu_clk);
}

int gpu2d_release(struct uio_info *info, struct inode *inode)
{
	if (IS_ERR(gpu_clk))
		return PTR_ERR(gpu_clk);

	clk_disable(gpu_clk);
	clk_put(gpu_clk);
	return 0;
}

static int gpu2d_mmap(struct uio_info *info, struct vm_area_struct *vma)
{
	int mi = vma->vm_pgoff;
	if (mi < 0)
		return -EINVAL;

	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	return remap_pfn_range(vma,
			       vma->vm_start,
			       info->mem[mi].addr >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
}

static struct uio_info gpu2d_info = {
	.name = "imx_gpu2d",
	.version = "1",
	.irq = MXC_INT_GPU2_IRQ,
	.open = gpu2d_open,
	.release = gpu2d_release,
	.mmap = gpu2d_mmap,
};

static struct platform_device mxc_gpu2d_device = {
	.name = "uio_pdrv_genirq",
	.dev = {
		.platform_data = &gpu2d_info,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	.num_resources = ARRAY_SIZE(mxc_gpu2d_resources),
	.resource = mxc_gpu2d_resources,
};

static inline void mxc_init_gpu2d(void)
{
	dma_alloc_coherent(&mxc_gpu2d_device.dev, SZ_8K, &mxc_gpu2d_resources[1].start, GFP_DMA);
	mxc_gpu2d_resources[1].end = mxc_gpu2d_resources[1].start + SZ_8K - 1;

	dma_alloc_coherent(&mxc_gpu2d_device.dev, 88 * SZ_1K, &mxc_gpu2d_resources[2].start, GFP_DMA);
	mxc_gpu2d_resources[2].end = mxc_gpu2d_resources[2].start + (88 * SZ_1K) - 1;

	platform_device_register(&mxc_gpu2d_device);
}
#else
static inline void mxc_init_gpu2d(void)
{
}
#endif

void __init mx51_init_irq(void)
{
	unsigned long tzic_addr;

	if (cpu_is_mx51_rev(CHIP_REV_2_0) < 0)
		tzic_addr = TZIC_BASE_ADDR_T01;
	else
		tzic_addr = TZIC_BASE_ADDR;

	mxc_tzic_init_irq(tzic_addr);
}

#define SCM_RD_DELAY	1000000 /* in nanoseconds */
#define SEC_TO_NANOSEC  1000000000 /*Second to nanoseconds */
static __init void mxc_init_scc_iram(void)
{
	uint32_t reg_value;
	uint32_t reg_mask = 0;
	uint8_t *UMID_base;
	uint32_t *MAP_base;
	uint8_t i;
	uint32_t partition_no;
	uint32_t scc_partno;
	void *scm_ram_base;
	void *scc_base;
	uint8_t iram_partitions = 16;
	struct timespec stime;
	struct timespec curtime;
	long scm_rd_timeout = 0;
	long cur_ns = 0;
	long start_ns = 0;

	if (cpu_is_mx51_rev(CHIP_REV_2_0) < 0)
		iram_partitions = 12;

	scc_base = ioremap((uint32_t) SCC_BASE_ADDR, 0x140);
	if (scc_base == NULL) {
		printk(KERN_ERR "FAILED TO MAP IRAM REGS\n");
		return;
	}
	scm_ram_base = ioremap((uint32_t) IRAM_BASE_ADDR, IRAM_SIZE);
	if (scm_ram_base == NULL) {
		printk(KERN_ERR "FAILED TO MAP IRAM\n");
		return;
	}

	/* Wait for any running SCC operations to finish or fail */
	getnstimeofday(&stime);
	do {
		reg_value = __raw_readl(scc_base + SCM_STATUS_REG);
		getnstimeofday(&curtime);
		if (curtime.tv_nsec > stime.tv_nsec)
			scm_rd_timeout = curtime.tv_nsec - stime.tv_nsec;
		else{
			/*Converted second to nanosecond and add to
			nsec when current nanosec is less than
			start time nanosec.*/
			cur_ns = (curtime.tv_sec * SEC_TO_NANOSEC) +
			curtime.tv_nsec;
			start_ns = (stime.tv_sec * SEC_TO_NANOSEC) +
				stime.tv_nsec;
			scm_rd_timeout = cur_ns - start_ns;
		}
	} while (((reg_value & SCM_STATUS_SRS_MASK) != SCM_STATUS_SRS_READY)
	&& ((reg_value & SCM_STATUS_SRS_MASK) != SCM_STATUS_SRS_FAIL));

	/* Check for failures */
	if ((reg_value & SCM_STATUS_SRS_MASK) != SCM_STATUS_SRS_READY) {
		/* Special message for bad secret key fuses */
		if (reg_value & SCM_STATUS_KST_BAD_KEY)
			printk(KERN_ERR "INVALID SCC KEY FUSE PATTERN\n");
		else
		    printk(KERN_ERR "SECURE RAM FAILURE\n");

		iounmap(scm_ram_base);
		iounmap(scc_base);
		return;
	}

	scm_rd_timeout = 0;
	/* Release final two partitions for SCC2 driver */
	scc_partno = iram_partitions - (SCC_IRAM_SIZE / SZ_8K);
	for (partition_no = scc_partno; partition_no < iram_partitions;
	     partition_no++) {
		reg_value = (((partition_no << SCM_ZCMD_PART_SHIFT) &
			SCM_ZCMD_PART_MASK) | ((0x03 << SCM_ZCMD_CCMD_SHIFT) &
			SCM_ZCMD_CCMD_MASK));
		__raw_writel(reg_value, scc_base + SCM_ZCMD_REG);
		udelay(1);
		/* Wait for zeroization to complete */
		getnstimeofday(&stime);
		do {
			reg_value = __raw_readl(scc_base + SCM_STATUS_REG);
			getnstimeofday(&curtime);
			if (curtime.tv_nsec > stime.tv_nsec)
				scm_rd_timeout = curtime.tv_nsec -
				stime.tv_nsec;
			else {
				/*Converted second to nanosecond and add to
				nsec when current nanosec is less than
				start time nanosec.*/
				cur_ns = (curtime.tv_sec * SEC_TO_NANOSEC) +
				curtime.tv_nsec;
				start_ns = (stime.tv_sec * SEC_TO_NANOSEC) +
					stime.tv_nsec;
				scm_rd_timeout = cur_ns - start_ns;
			}
		} while (((reg_value & SCM_STATUS_SRS_MASK) !=
		SCM_STATUS_SRS_READY) && ((reg_value & SCM_STATUS_SRS_MASK) !=
		SCM_STATUS_SRS_FAIL) && (scm_rd_timeout <= SCM_RD_DELAY));

		if (scm_rd_timeout > SCM_RD_DELAY)
			printk(KERN_ERR "SCM Status Register Read timeout"
			"for Partition No:%d", partition_no);

		if ((reg_value & SCM_STATUS_SRS_MASK) != SCM_STATUS_SRS_READY)
			break;
	}

	/*Check all expected partitions released */
	reg_value = __raw_readl(scc_base + SCM_PART_OWNERS_REG);
	if ((reg_value & reg_mask) != 0) {
		printk(KERN_ERR "FAILED TO RELEASE IRAM PARTITION\n");
		iounmap(scm_ram_base);
		iounmap(scc_base);
		return;
	}
	reg_mask = 0;
	scm_rd_timeout = 0;
	/* Allocate remaining partitions for general use */
	for (partition_no = 0; partition_no < scc_partno; partition_no++) {
		/* Supervisor mode claims a partition for it's own use
		by writing zero to SMID register.*/
		__raw_writel(0, scc_base + (SCM_SMID0_REG + 8 * partition_no));

		/* Wait for any zeroization to complete */
		getnstimeofday(&stime);
		do {
			reg_value = __raw_readl(scc_base + SCM_STATUS_REG);
			getnstimeofday(&curtime);
			if (curtime.tv_nsec > stime.tv_nsec)
				scm_rd_timeout = curtime.tv_nsec -
				stime.tv_nsec;
			else{
				/*Converted second to nanosecond and add to
				nsec when current nanosec is less than
				start time nanosec.*/
				cur_ns = (curtime.tv_sec * SEC_TO_NANOSEC) +
				curtime.tv_nsec;
				start_ns = (stime.tv_sec * SEC_TO_NANOSEC) +
					stime.tv_nsec;
				scm_rd_timeout = cur_ns - start_ns;
			}
		} while (((reg_value & SCM_STATUS_SRS_MASK) !=
		SCM_STATUS_SRS_READY) && ((reg_value & SCM_STATUS_SRS_MASK) !=
		SCM_STATUS_SRS_FAIL) && (scm_rd_timeout <= SCM_RD_DELAY));

		if (scm_rd_timeout > SCM_RD_DELAY)
			printk(KERN_ERR "SCM Status Register Read timeout"
			"for Partition No:%d", partition_no);

		if ((reg_value & SCM_STATUS_SRS_MASK) != SCM_STATUS_SRS_READY)
			break;
		/* Set UMID=0 and permissions for universal data
		read/write access */
		MAP_base = scm_ram_base + (partition_no * 0x2000);
		UMID_base = (uint8_t *) MAP_base + 0x10;
		for (i = 0; i < 16; i++)
			UMID_base[i] = 0;

		MAP_base[0] = (SCM_PERM_NO_ZEROIZE | SCM_PERM_HD_SUP_DISABLE |
			SCM_PERM_HD_READ | SCM_PERM_HD_WRITE |
			SCM_PERM_HD_EXECUTE | SCM_PERM_TH_READ |
			SCM_PERM_TH_WRITE);
		reg_mask |= (3 << (2 * (partition_no)));
	}

	/* Check all expected partitions allocated */
	reg_value = __raw_readl(scc_base + SCM_PART_OWNERS_REG);
	if ((reg_value & reg_mask) != reg_mask) {
		printk(KERN_ERR "FAILED TO ACQUIRE IRAM PARTITION\n");
		iounmap(scm_ram_base);
		iounmap(scc_base);
		return;
	}

	iounmap(scm_ram_base);
	iounmap(scc_base);
	printk(KERN_INFO "IRAM READY\n");
	iram_ready = 1;
}

int __init mxc_init_devices(void)
{
	unsigned long addr;

	iram_alloc(VPU_IRAM_SIZE, &addr);
	vpu_resources[0].start = addr;
	vpu_resources[0].end = addr + VPU_IRAM_SIZE - 1;

	mxc_init_scc_iram();
	mxc_init_gpu2d();
	return 0;
}
postcore_initcall(mxc_init_devices);

