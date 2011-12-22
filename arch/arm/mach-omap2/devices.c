/*
 * linux/arch/arm/mach-omap2/devices.c
 *
 * OMAP2 platform device setup/initialization
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/davinci_emac.h>
#include <linux/cpsw.h>
#include <linux/etherdevice.h>
#include <linux/dma-mapping.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/board-am335xevm.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>
#include <asm/pmu.h>

#ifdef	CONFIG_OMAP3_EDMA
#include <mach/edma.h>
#endif

#include <asm/hardware/asp.h>

#include <plat/tc.h>
#include <plat/board.h>
#include <plat/mcbsp.h>
#include <mach/gpio.h>
#include <plat/mmc.h>
#include <plat/dma.h>
#include <plat/omap_hwmod.h>
#include <plat/omap_device.h>
#include <plat/omap4-keypad.h>

/* LCD controller similar DA8xx */
#include <video/da8xx-fb.h>

#include "mux.h"
#include "control.h"
#include "devices.h"

#define L3_MODULES_MAX_LEN 12
#define L3_MODULES 3

void am33xx_cpsw_init(void);

static int __init omap3_l3_init(void)
{
	int l;
	struct omap_hwmod *oh;
	struct platform_device *pdev;
	char oh_name[L3_MODULES_MAX_LEN];

	/*
	 * To avoid code running on other OMAPs in
	 * multi-omap builds
	 */
	if (!(cpu_is_omap34xx()))
		return -ENODEV;

	l = snprintf(oh_name, L3_MODULES_MAX_LEN, "l3_main");

	oh = omap_hwmod_lookup(oh_name);

	if (!oh)
		pr_err("could not look up %s\n", oh_name);

	pdev = omap_device_build("omap_l3_smx", 0, oh, NULL, 0,
							   NULL, 0, 0);

	WARN(IS_ERR(pdev), "could not build omap_device for %s\n", oh_name);

	return IS_ERR(pdev) ? PTR_ERR(pdev) : 0;
}
postcore_initcall(omap3_l3_init);

static int __init omap4_l3_init(void)
{
	int l, i;
	struct omap_hwmod *oh[3];
	struct platform_device *pdev;
	char oh_name[L3_MODULES_MAX_LEN];

	/* If dtb is there, the devices will be created dynamically */
	if (of_have_populated_dt())
		return -ENODEV;

	/*
	 * To avoid code running on other OMAPs in
	 * multi-omap builds
	 */
	if (!(cpu_is_omap44xx()))
		return -ENODEV;

	for (i = 0; i < L3_MODULES; i++) {
		l = snprintf(oh_name, L3_MODULES_MAX_LEN, "l3_main_%d", i+1);

		oh[i] = omap_hwmod_lookup(oh_name);
		if (!(oh[i]))
			pr_err("could not look up %s\n", oh_name);
	}

	pdev = omap_device_build_ss("omap_l3_noc", 0, oh, 3, NULL,
						     0, NULL, 0, 0);

	WARN(IS_ERR(pdev), "could not build omap_device for %s\n", oh_name);

	return IS_ERR(pdev) ? PTR_ERR(pdev) : 0;
}
postcore_initcall(omap4_l3_init);

#if defined(CONFIG_VIDEO_OMAP2) || defined(CONFIG_VIDEO_OMAP2_MODULE)

static struct resource omap2cam_resources[] = {
	{
		.start		= OMAP24XX_CAMERA_BASE,
		.end		= OMAP24XX_CAMERA_BASE + 0xfff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_CAM_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device omap2cam_device = {
	.name		= "omap24xxcam",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(omap2cam_resources),
	.resource	= omap2cam_resources,
};
#endif
#define L4_PER_LCDC_PHYS        0x4830E000

static struct resource am33xx_lcdc_resources[] = {
	[0] = { /* registers */
		.start  = L4_PER_LCDC_PHYS,
		.end    = L4_PER_LCDC_PHYS + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = { /* interrupt */
		.start  = AM33XX_IRQ_LCD,
		.end    = AM33XX_IRQ_LCD,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device am33xx_lcdc_device = {
	.name		= "da8xx_lcdc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(am33xx_lcdc_resources),
	.resource	= am33xx_lcdc_resources,
};

void __init am33xx_register_lcdc(struct da8xx_lcdc_platform_data *pdata)
{
	int ret;

	am33xx_lcdc_device.dev.platform_data = pdata;

	ret = platform_device_register(&am33xx_lcdc_device);
	if (ret)
		pr_warning("am33xx_register_lcdc: lcdc registration failed: %d\n",
				ret);

}

#if defined (CONFIG_SND_AM335X_SOC_EVM)
static struct resource am335x_mcasp1_resource[] = {
	{
		.name = "mcasp1",
		.start = AM33XX_ASP1_BASE,
		.end = AM33XX_ASP1_BASE + (SZ_1K * 12) - 1,
		.flags = IORESOURCE_MEM,
	},
	/* TX event */
	{
		.start = AM33XX_DMA_MCASP1_X,
		.end = AM33XX_DMA_MCASP1_X,
		.flags = IORESOURCE_DMA,
	},
	/* RX event */
	{
		.start = AM33XX_DMA_MCASP1_R,
		.end = AM33XX_DMA_MCASP1_R,
		.flags = IORESOURCE_DMA,
	},
};

static struct platform_device am335x_mcasp1_device = {
	.name = "davinci-mcasp",
	.id = 1,
	.num_resources = ARRAY_SIZE(am335x_mcasp1_resource),
	.resource = am335x_mcasp1_resource,
};

void __init am335x_register_mcasp1(struct snd_platform_data *pdata)
{
	am335x_mcasp1_device.dev.platform_data = pdata;
	platform_device_register(&am335x_mcasp1_device);
}

#else
void __init am335x_register_mcasp1(struct snd_platform_data *pdata) {}
#endif

#if defined(CONFIG_SND_AM33XX_SOC)
struct platform_device am33xx_pcm_device = {
	.name		= "davinci-pcm-audio",
	.id		= -1,
};

static void am33xx_init_pcm(void)
{
	platform_device_register(&am33xx_pcm_device);
}

#else
static inline void am33xx_init_pcm(void) {}
#endif

static struct resource omap3isp_resources[] = {
	{
		.start		= OMAP3430_ISP_BASE,
		.end		= OMAP3430_ISP_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CCP2_BASE,
		.end		= OMAP3430_ISP_CCP2_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CCDC_BASE,
		.end		= OMAP3430_ISP_CCDC_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_HIST_BASE,
		.end		= OMAP3430_ISP_HIST_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_H3A_BASE,
		.end		= OMAP3430_ISP_H3A_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_PREV_BASE,
		.end		= OMAP3430_ISP_PREV_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_RESZ_BASE,
		.end		= OMAP3430_ISP_RESZ_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_SBL_BASE,
		.end		= OMAP3430_ISP_SBL_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CSI2A_REGS1_BASE,
		.end		= OMAP3430_ISP_CSI2A_REGS1_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CSIPHY2_BASE,
		.end		= OMAP3430_ISP_CSIPHY2_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3630_ISP_CSI2A_REGS2_BASE,
		.end		= OMAP3630_ISP_CSI2A_REGS2_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3630_ISP_CSI2C_REGS1_BASE,
		.end		= OMAP3630_ISP_CSI2C_REGS1_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3630_ISP_CSIPHY1_BASE,
		.end		= OMAP3630_ISP_CSIPHY1_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3630_ISP_CSI2C_REGS2_BASE,
		.end		= OMAP3630_ISP_CSI2C_REGS2_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_34XX_CAM_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device omap3isp_device = {
	.name		= "omap3isp",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(omap3isp_resources),
	.resource	= omap3isp_resources,
};

int omap3_init_camera(struct isp_platform_data *pdata)
{
	omap3isp_device.dev.platform_data = pdata;
	return platform_device_register(&omap3isp_device);
}

static inline void omap_init_camera(void)
{
#if defined(CONFIG_VIDEO_OMAP2) || defined(CONFIG_VIDEO_OMAP2_MODULE)
	if (cpu_is_omap24xx())
		platform_device_register(&omap2cam_device);
#endif
}

int __init omap4_keyboard_init(struct omap4_keypad_platform_data
			*sdp4430_keypad_data, struct omap_board_data *bdata)
{
	struct platform_device *pdev;
	struct omap_hwmod *oh;
	struct omap4_keypad_platform_data *keypad_data;
	unsigned int id = -1;
	char *oh_name = "kbd";
	char *name = "omap4-keypad";

	oh = omap_hwmod_lookup(oh_name);
	if (!oh) {
		pr_err("Could not look up %s\n", oh_name);
		return -ENODEV;
	}

	keypad_data = sdp4430_keypad_data;

	pdev = omap_device_build(name, id, oh, keypad_data,
			sizeof(struct omap4_keypad_platform_data), NULL, 0, 0);

	if (IS_ERR(pdev)) {
		WARN(1, "Can't build omap_device for %s:%s.\n",
						name, oh->name);
		return PTR_ERR(pdev);
	}
	oh->mux = omap_hwmod_mux_init(bdata->pads, bdata->pads_cnt);

	return 0;
}

#if defined(CONFIG_OMAP_MBOX_FWK) || defined(CONFIG_OMAP_MBOX_FWK_MODULE)
static inline void omap_init_mbox(void)
{
	struct omap_hwmod *oh;
	struct platform_device *pdev;

	oh = omap_hwmod_lookup("mailbox");
	if (!oh) {
		pr_err("%s: unable to find hwmod\n", __func__);
		return;
	}

	pdev = omap_device_build("omap-mailbox", -1, oh, NULL, 0, NULL, 0, 0);
	WARN(IS_ERR(pdev), "%s: could not build device, err %ld\n",
						__func__, PTR_ERR(pdev));
}
#else
static inline void omap_init_mbox(void) { }
#endif /* CONFIG_OMAP_MBOX_FWK */

static inline void omap_init_sti(void) {}

#if defined(CONFIG_SND_SOC) || defined(CONFIG_SND_SOC_MODULE)

static struct platform_device omap_pcm = {
	.name	= "omap-pcm-audio",
	.id	= -1,
};

/*
 * OMAP2420 has 2 McBSP ports
 * OMAP2430 has 5 McBSP ports
 * OMAP3 has 5 McBSP ports
 * OMAP4 has 4 McBSP ports
 */
OMAP_MCBSP_PLATFORM_DEVICE(1);
OMAP_MCBSP_PLATFORM_DEVICE(2);
OMAP_MCBSP_PLATFORM_DEVICE(3);
OMAP_MCBSP_PLATFORM_DEVICE(4);
OMAP_MCBSP_PLATFORM_DEVICE(5);

static void omap_init_audio(void)
{
	platform_device_register(&omap_mcbsp1);
	platform_device_register(&omap_mcbsp2);
	if (cpu_is_omap243x() || cpu_is_omap34xx() || cpu_is_omap44xx()) {
		platform_device_register(&omap_mcbsp3);
		platform_device_register(&omap_mcbsp4);
	}
	if (cpu_is_omap243x() || cpu_is_omap34xx())
		platform_device_register(&omap_mcbsp5);

	platform_device_register(&omap_pcm);
}

#else
static inline void omap_init_audio(void) {}
#endif

#if defined(CONFIG_SPI_OMAP24XX) || defined(CONFIG_SPI_OMAP24XX_MODULE)

#include <plat/mcspi.h>

static int omap_mcspi_init(struct omap_hwmod *oh, void *unused)
{
	struct platform_device *pdev;
	char *name = "omap2_mcspi";
	struct omap2_mcspi_platform_config *pdata;
	static int spi_num;
	struct omap2_mcspi_dev_attr *mcspi_attrib = oh->dev_attr;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("Memory allocation for McSPI device failed\n");
		return -ENOMEM;
	}

	pdata->num_cs = mcspi_attrib->num_chipselect;
	switch (oh->class->rev) {
	case OMAP2_MCSPI_REV:
	case OMAP3_MCSPI_REV:
			pdata->regs_offset = 0;
			break;
	case OMAP4_MCSPI_REV:
			pdata->regs_offset = OMAP4_MCSPI_REG_OFFSET;
			if (cpu_is_am33xx())
				pdata->dma_not_enabled = true;
			break;
	default:
			pr_err("Invalid McSPI Revision value\n");
			return -EINVAL;
	}

	spi_num++;
	pdev = omap_device_build(name, spi_num, oh, pdata,
				sizeof(*pdata),	NULL, 0, 0);
	WARN(IS_ERR(pdev), "Can't build omap_device for %s:%s\n",
				name, oh->name);
	kfree(pdata);
	return 0;
}

static void omap_init_mcspi(void)
{
	omap_hwmod_for_each_by_class("mcspi", omap_mcspi_init, NULL);
}

#else
static inline void omap_init_mcspi(void) {}
#endif

static struct resource omap2_pmu_resource = {
	.start	= 3,
	.end	= 3,
	.flags	= IORESOURCE_IRQ,
};

static struct resource omap3_pmu_resource = {
	.start	= INT_34XX_BENCH_MPU_EMUL,
	.end	= INT_34XX_BENCH_MPU_EMUL,
	.flags	= IORESOURCE_IRQ,
};

static struct platform_device omap_pmu_device = {
	.name		= "arm-pmu",
	.id		= ARM_PMU_DEVICE_CPU,
	.num_resources	= 1,
};

static void omap_init_pmu(void)
{
	if (cpu_is_omap24xx())
		omap_pmu_device.resource = &omap2_pmu_resource;
	else if (cpu_is_omap34xx())
		omap_pmu_device.resource = &omap3_pmu_resource;
	else
		return;

	platform_device_register(&omap_pmu_device);
}


#if defined(CONFIG_CRYPTO_DEV_OMAP_SHAM) || defined(CONFIG_CRYPTO_DEV_OMAP_SHAM_MODULE)

#ifdef CONFIG_ARCH_OMAP2
static struct resource omap2_sham_resources[] = {
	{
		.start	= OMAP24XX_SEC_SHA1MD5_BASE,
		.end	= OMAP24XX_SEC_SHA1MD5_BASE + 0x64,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_24XX_SHA1MD5,
		.flags	= IORESOURCE_IRQ,
	}
};
static int omap2_sham_resources_sz = ARRAY_SIZE(omap2_sham_resources);
#else
#define omap2_sham_resources		NULL
#define omap2_sham_resources_sz		0
#endif

#ifdef CONFIG_ARCH_OMAP3
static struct resource omap3_sham_resources[] = {
	{
		.start	= OMAP34XX_SEC_SHA1MD5_BASE,
		.end	= OMAP34XX_SEC_SHA1MD5_BASE + 0x64,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_34XX_SHA1MD52_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= OMAP34XX_DMA_SHA1MD5_RX,
		.flags	= IORESOURCE_DMA,
	}
};
static int omap3_sham_resources_sz = ARRAY_SIZE(omap3_sham_resources);
#else
#define omap3_sham_resources		NULL
#define omap3_sham_resources_sz		0
#endif

static struct platform_device sham_device = {
	.name		= "omap-sham",
	.id		= -1,
};

static void omap_init_sham(void)
{
	if (cpu_is_omap24xx()) {
		sham_device.resource = omap2_sham_resources;
		sham_device.num_resources = omap2_sham_resources_sz;
	} else if (cpu_is_omap34xx()) {
		sham_device.resource = omap3_sham_resources;
		sham_device.num_resources = omap3_sham_resources_sz;
	} else {
		pr_err("%s: platform not supported\n", __func__);
		return;
	}
	platform_device_register(&sham_device);
}
#else
static inline void omap_init_sham(void) { }
#endif

#if defined(CONFIG_CRYPTO_DEV_OMAP_AES) || defined(CONFIG_CRYPTO_DEV_OMAP_AES_MODULE)

#ifdef CONFIG_ARCH_OMAP2
static struct resource omap2_aes_resources[] = {
	{
		.start	= OMAP24XX_SEC_AES_BASE,
		.end	= OMAP24XX_SEC_AES_BASE + 0x4C,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= OMAP24XX_DMA_AES_TX,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= OMAP24XX_DMA_AES_RX,
		.flags	= IORESOURCE_DMA,
	}
};
static int omap2_aes_resources_sz = ARRAY_SIZE(omap2_aes_resources);
#else
#define omap2_aes_resources		NULL
#define omap2_aes_resources_sz		0
#endif

#ifdef CONFIG_ARCH_OMAP3
static struct resource omap3_aes_resources[] = {
	{
		.start	= OMAP34XX_SEC_AES_BASE,
		.end	= OMAP34XX_SEC_AES_BASE + 0x4C,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= OMAP34XX_DMA_AES2_TX,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= OMAP34XX_DMA_AES2_RX,
		.flags	= IORESOURCE_DMA,
	}
};
static int omap3_aes_resources_sz = ARRAY_SIZE(omap3_aes_resources);
#else
#define omap3_aes_resources		NULL
#define omap3_aes_resources_sz		0
#endif

static struct platform_device aes_device = {
	.name		= "omap-aes",
	.id		= -1,
};

static void omap_init_aes(void)
{
	if (cpu_is_omap24xx()) {
		aes_device.resource = omap2_aes_resources;
		aes_device.num_resources = omap2_aes_resources_sz;
	} else if (cpu_is_omap34xx()) {
		aes_device.resource = omap3_aes_resources;
		aes_device.num_resources = omap3_aes_resources_sz;
	} else {
		pr_err("%s: platform not supported\n", __func__);
		return;
	}
	platform_device_register(&aes_device);
}

#else
static inline void omap_init_aes(void) { }
#endif

/*-------------------------------------------------------------------------*/

#if defined(CONFIG_MMC_OMAP) || defined(CONFIG_MMC_OMAP_MODULE)

static inline void omap242x_mmc_mux(struct omap_mmc_platform_data
							*mmc_controller)
{
	if ((mmc_controller->slots[0].switch_pin > 0) && \
		(mmc_controller->slots[0].switch_pin < OMAP_MAX_GPIO_LINES))
		omap_mux_init_gpio(mmc_controller->slots[0].switch_pin,
					OMAP_PIN_INPUT_PULLUP);
	if ((mmc_controller->slots[0].gpio_wp > 0) && \
		(mmc_controller->slots[0].gpio_wp < OMAP_MAX_GPIO_LINES))
		omap_mux_init_gpio(mmc_controller->slots[0].gpio_wp,
					OMAP_PIN_INPUT_PULLUP);

	omap_mux_init_signal("sdmmc_cmd", 0);
	omap_mux_init_signal("sdmmc_clki", 0);
	omap_mux_init_signal("sdmmc_clko", 0);
	omap_mux_init_signal("sdmmc_dat0", 0);
	omap_mux_init_signal("sdmmc_dat_dir0", 0);
	omap_mux_init_signal("sdmmc_cmd_dir", 0);
	if (mmc_controller->slots[0].caps & MMC_CAP_4_BIT_DATA) {
		omap_mux_init_signal("sdmmc_dat1", 0);
		omap_mux_init_signal("sdmmc_dat2", 0);
		omap_mux_init_signal("sdmmc_dat3", 0);
		omap_mux_init_signal("sdmmc_dat_dir1", 0);
		omap_mux_init_signal("sdmmc_dat_dir2", 0);
		omap_mux_init_signal("sdmmc_dat_dir3", 0);
	}

	/*
	 * Use internal loop-back in MMC/SDIO Module Input Clock
	 * selection
	 */
	if (mmc_controller->slots[0].internal_clock) {
		u32 v = omap_ctrl_readl(OMAP2_CONTROL_DEVCONF0);
		v |= (1 << 24);
		omap_ctrl_writel(v, OMAP2_CONTROL_DEVCONF0);
	}
}

void __init omap242x_init_mmc(struct omap_mmc_platform_data **mmc_data)
{
	char *name = "mmci-omap";

	if (!mmc_data[0]) {
		pr_err("%s fails: Incomplete platform data\n", __func__);
		return;
	}

	omap242x_mmc_mux(mmc_data[0]);
	omap_mmc_add(name, 0, OMAP2_MMC1_BASE, OMAP2420_MMC_SIZE,
					INT_24XX_MMC_IRQ, mmc_data[0]);
}

#endif

/*-------------------------------------------------------------------------*/

#if defined(CONFIG_HDQ_MASTER_OMAP) || defined(CONFIG_HDQ_MASTER_OMAP_MODULE)
#if defined(CONFIG_SOC_OMAP2430) || defined(CONFIG_SOC_OMAP3430)
#define OMAP_HDQ_BASE	0x480B2000
#endif
static struct resource omap_hdq_resources[] = {
	{
		.start		= OMAP_HDQ_BASE,
		.end		= OMAP_HDQ_BASE + 0x1C,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_HDQ_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct platform_device omap_hdq_dev = {
	.name = "omap_hdq",
	.id = 0,
	.dev = {
		.platform_data = NULL,
	},
	.num_resources	= ARRAY_SIZE(omap_hdq_resources),
	.resource	= omap_hdq_resources,
};
static inline void omap_hdq_init(void)
{
	(void) platform_device_register(&omap_hdq_dev);
}
#else
static inline void omap_hdq_init(void) {}
#endif

/*---------------------------------------------------------------------------*/

#if defined(CONFIG_VIDEO_OMAP2_VOUT) || \
	defined(CONFIG_VIDEO_OMAP2_VOUT_MODULE)
#if defined(CONFIG_FB_OMAP2) || defined(CONFIG_FB_OMAP2_MODULE)
static struct resource omap_vout_resource[3 - CONFIG_FB_OMAP2_NUM_FBS] = {
};
#else
static struct resource omap_vout_resource[2] = {
};
#endif

static struct platform_device omap_vout_device = {
	.name		= "omap_vout",
	.num_resources	= ARRAY_SIZE(omap_vout_resource),
	.resource 	= &omap_vout_resource[0],
	.id		= -1,
};
static void omap_init_vout(void)
{
	if (platform_device_register(&omap_vout_device) < 0)
		printk(KERN_ERR "Unable to register OMAP-VOUT device\n");
}
#else
static inline void omap_init_vout(void) {}
#endif

#if defined(CONFIG_SOC_OMAPAM33XX) && defined(CONFIG_OMAP3_EDMA)

#define AM33XX_TPCC_BASE		0x49000000
#define AM33XX_TPTC0_BASE		0x49800000
#define AM33XX_TPTC1_BASE		0x49900000
#define AM33XX_TPTC2_BASE		0x49a00000

#define AM33XX_SCM_BASE_EDMA		0x00000f90

static struct resource am33xx_edma_resources[] = {
	{
		.name	= "edma_cc0",
		.start	= AM33XX_TPCC_BASE,
		.end	= AM33XX_TPCC_BASE + SZ_32K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc0",
		.start	= AM33XX_TPTC0_BASE,
		.end	= AM33XX_TPTC0_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc1",
		.start	= AM33XX_TPTC1_BASE,
		.end	= AM33XX_TPTC1_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc2",
		.start	= AM33XX_TPTC2_BASE,
		.end	= AM33XX_TPTC2_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma0",
		.start	= AM33XX_IRQ_TPCC0_INT_PO0,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "edma0_err",
		.start	= AM33XX_IRQ_TPCC0_ERRINT_PO,
		.flags	= IORESOURCE_IRQ,
	},
};

static const s16 am33xx_dma_rsv_chans[][2] = {
	/* (offset, number) */
	{0, 2},
	{14, 2},
	{26, 6},
	{48, 4},
	{56, 8},
	{-1, -1}
};

static const s16 am33xx_dma_rsv_slots[][2] = {
	/* (offset, number) */
	{0, 2},
	{14, 2},
	{26, 6},
	{48, 4},
	{56, 8},
	{64, 127},
	{-1, -1}
};

/* Three Transfer Controllers on AM33XX */
static const s8 am33xx_queue_tc_mapping[][2] = {
	/* {event queue no, TC no} */
	{0, 0},
	{1, 1},
	{2, 2},
	{-1, -1}
};

static const s8 am33xx_queue_priority_mapping[][2] = {
	/* {event queue no, Priority} */
	{0, 0},
	{1, 1},
	{2, 2},
	{-1, -1}
};

static struct event_to_channel_map am33xx_xbar_event_mapping[] = {
	/* {xbar event no, Channel} */
	{1, 12},	/* SDTXEVT1 -> MMCHS2 */
	{2, 13},	/* SDRXEVT1 -> MMCHS2 */
	{3, -1},
	{4, -1},
	{5, -1},
	{6, -1},
	{7, -1},
	{8, -1},
	{9, -1},
	{10, -1},
	{11, -1},
	{12, -1},
	{13, -1},
	{14, -1},
	{15, -1},
	{16, -1},
	{17, -1},
	{18, -1},
	{19, -1},
	{20, -1},
	{21, -1},
	{22, -1},
	{23, -1},
	{24, -1},
	{25, -1},
	{26, -1},
	{27, -1},
	{28, -1},
	{29, -1},
	{30, -1},
	{31, -1},
	{-1, -1}
};

/**
 * map_xbar_event_to_channel - maps a crossbar event to a DMA channel
 * according to the configuration provided
 * @event: the event number for which mapping is required
 * @channel: channel being activated
 * @xbar_event_mapping: array that has the event to channel map
 *
 * Events that are routed by default are not mapped. Only events that
 * are crossbar mapped are routed to available channels according to
 * the configuration provided
 *
 * Returns zero on success, else negative errno.
 */
int map_xbar_event_to_channel(unsigned event, unsigned *channel,
			struct event_to_channel_map *xbar_event_mapping)
{
	unsigned ctrl = 0;
	unsigned xbar_evt_no = 0;
	unsigned val = 0;
	unsigned offset = 0;
	unsigned mask = 0;

	ctrl = EDMA_CTLR(event);
	xbar_evt_no = event - (edma_info[ctrl]->num_channels);

	if (event < edma_info[ctrl]->num_channels) {
		*channel = event;
	} else if (event < edma_info[ctrl]->num_events) {
		*channel = xbar_event_mapping[xbar_evt_no].channel_no;
		/* confirm the range */
		if (*channel < EDMA_MAX_DMACH)
			clear_bit(*channel, edma_info[ctrl]->edma_unused);
		mask = (*channel)%4;
		offset = (*channel)/4;
		offset *= 4;
		offset += mask;
		val = (unsigned)__raw_readl(AM33XX_CTRL_REGADDR(
					AM33XX_SCM_BASE_EDMA + offset));
		val = val & (~(0xFF));
		val = val | (xbar_event_mapping[xbar_evt_no].xbar_event_no);
		__raw_writel(val,
			AM33XX_CTRL_REGADDR(AM33XX_SCM_BASE_EDMA + offset));
		return 0;
	} else {
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(map_xbar_event_to_channel);

static struct edma_soc_info am33xx_edma_info[] = {
	{
		.n_channel		= 64,
		.n_region		= 4,
		.n_slot			= 256,
		.n_tc			= 3,
		.n_cc			= 1,
		.rsv_chans		= am33xx_dma_rsv_chans,
		.rsv_slots		= am33xx_dma_rsv_slots,
		.queue_tc_mapping	= am33xx_queue_tc_mapping,
		.queue_priority_mapping	= am33xx_queue_priority_mapping,
		.is_xbar		= 1,
		.n_events		= 95,
		.xbar_event_mapping	= am33xx_xbar_event_mapping,
		.map_xbar_channel	= map_xbar_event_to_channel,
	},
};

static struct platform_device am33xx_edma_device = {
	.name		= "edma",
	.id		= -1,
	.dev = {
		.platform_data = am33xx_edma_info,
	},
	.num_resources	= ARRAY_SIZE(am33xx_edma_resources),
	.resource	= am33xx_edma_resources,
};

int __init am33xx_register_edma(void)
{
	struct platform_device *pdev;
	static struct clk *edma_clk;

	if (cpu_is_am33xx())
		pdev = &am33xx_edma_device;
	else {
		pr_err("%s: platform not supported\n", __func__);
		return -ENODEV;
	}

	edma_clk = clk_get(NULL, "tpcc_ick");
	if (IS_ERR(edma_clk)) {
		printk(KERN_ERR "EDMA: Failed to get clock\n");
		return -EBUSY;
	}
	clk_enable(edma_clk);
	edma_clk = clk_get(NULL, "tptc0_ick");
	if (IS_ERR(edma_clk)) {
		printk(KERN_ERR "EDMA: Failed to get clock\n");
		return -EBUSY;
	}
	clk_enable(edma_clk);
	edma_clk = clk_get(NULL, "tptc1_ick");
	if (IS_ERR(edma_clk)) {
		printk(KERN_ERR "EDMA: Failed to get clock\n");
		return -EBUSY;
	}
	clk_enable(edma_clk);
	edma_clk = clk_get(NULL, "tptc2_ick");
	if (IS_ERR(edma_clk)) {
		printk(KERN_ERR "EDMA: Failed to get clock\n");
		return -EBUSY;
	}
	clk_enable(edma_clk);

	return platform_device_register(pdev);
}

#else
static inline void am33xx_register_edma(void) {}
#endif

/*-------------------------------------------------------------------------*/

static int __init omap2_init_devices(void)
{
	/*
	 * please keep these calls, and their implementations above,
	 * in alphabetical order so they're easier to sort through.
	 */
	omap_init_audio();
	omap_init_camera();
	omap_init_mbox();
	omap_init_mcspi();
	omap_init_pmu();
	omap_hdq_init();
	omap_init_sti();
	omap_init_sham();
	omap_init_aes();
	omap_init_vout();
	am33xx_register_edma();
	am33xx_init_pcm();

	return 0;
}
arch_initcall(omap2_init_devices);

#define AM33XX_CPSW_BASE		(0x4A100000)
#define AM33XX_CPSW_MDIO_BASE		(0x4A101000)
#define AM33XX_CPSW_SS_BASE		(0x4A101200)
#define AM33XX_EMAC_MDIO_FREQ		(1000000)

static u64 am33xx_cpsw_dmamask = DMA_BIT_MASK(32);
/* TODO : Verify the offsets */
static struct cpsw_slave_data am33xx_cpsw_slaves[] = {
	{
		.slave_reg_ofs  = 0x208,
		.sliver_reg_ofs = 0xd80,
		.phy_id		= "0:00",
	},
	{
		.slave_reg_ofs  = 0x308,
		.sliver_reg_ofs = 0xdc0,
		.phy_id		= "0:01",
	},
};

static struct cpsw_platform_data am33xx_cpsw_pdata = {
	.ss_reg_ofs		= 0x1200,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 2,
	.slave_data		= am33xx_cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs      = 0x108,
	.hw_stats_reg_ofs       = 0x900,
	.bd_ram_ofs		= 0x2000,
	.bd_ram_size		= SZ_8K,
	.rx_descs               = 64,
	.mac_control            = BIT(5), /* MIIEN */
	.gigabit_en		= 1,
	.host_port_num		= 0,
	.no_bd_ram		= false,
	.version		= CPSW_VERSION_2,
};

static struct mdio_platform_data am33xx_cpsw_mdiopdata = {
	.bus_freq       = AM33XX_EMAC_MDIO_FREQ,
};

static struct resource am33xx_cpsw_mdioresources[] = {
	{
		.start  = AM33XX_CPSW_MDIO_BASE,
		.end    = AM33XX_CPSW_MDIO_BASE + SZ_256 - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device am33xx_cpsw_mdiodevice = {
	.name           = "davinci_mdio",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(am33xx_cpsw_mdioresources),
	.resource       = am33xx_cpsw_mdioresources,
	.dev.platform_data = &am33xx_cpsw_mdiopdata,
};

static struct resource am33xx_cpsw_resources[] = {
	{
		.start  = AM33XX_CPSW_BASE,
		.end    = AM33XX_CPSW_BASE + SZ_2K - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = AM33XX_CPSW_SS_BASE,
		.end    = AM33XX_CPSW_SS_BASE + SZ_256 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start	= AM33XX_IRQ_CPSW_C0_RX,
		.end	= AM33XX_IRQ_CPSW_C0_RX,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= AM33XX_IRQ_CPSW_RX,
		.end	= AM33XX_IRQ_CPSW_RX,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= AM33XX_IRQ_CPSW_TX,
		.end	= AM33XX_IRQ_CPSW_TX,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= AM33XX_IRQ_CPSW_C0,
		.end	= AM33XX_IRQ_CPSW_C0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device am33xx_cpsw_device = {
	.name		=	"cpsw",
	.id		=	0,
	.num_resources	=	ARRAY_SIZE(am33xx_cpsw_resources),
	.resource	=	am33xx_cpsw_resources,
	.dev		=	{
					.platform_data	= &am33xx_cpsw_pdata,
					.dma_mask	= &am33xx_cpsw_dmamask,
					.coherent_dma_mask = DMA_BIT_MASK(32),
				},
};

static unsigned char  am33xx_macid0[ETH_ALEN];
static unsigned char  am33xx_macid1[ETH_ALEN];
static unsigned int   am33xx_evmid;

/*
* am33xx_evmid_fillup - set up board evmid
* @evmid - evm id which needs to be configured
*
* This function is called to configure board evm id.
* IA Motor Control EVM needs special setting of MAC PHY Id.
* This function is called when IA Motor Control EVM is detected
* during boot-up.
*/
void am33xx_evmid_fillup(unsigned int evmid)
{
	am33xx_evmid = evmid;
	return;
}

/*
* am33xx_cpsw_macidfillup - setup mac adrresses
* @eeprommacid0 - mac id 0 which needs to be configured
* @eeprommacid1 - mac id 1 which needs to be configured
*
* This function is called to configure mac addresses.
* Mac addresses are read from eeprom and this function is called
* to store those mac adresses in am33xx_macid0 and am33xx_macid1.
* In case, mac address read from eFuse are invalid, mac addresses
* stored in these variable are used.
*/
void am33xx_cpsw_macidfillup(char *eeprommacid0, char *eeprommacid1)
{
	u32 i;

	/* Fillup these mac addresses with the mac adresses from eeprom */
	for (i = 0; i < ETH_ALEN; i++) {
		am33xx_macid0[i] = eeprommacid0[i];
		am33xx_macid1[i] = eeprommacid1[i];
	}

	return;
}

void am33xx_cpsw_init(void)
{
	u32 mac_lo, mac_hi;
	u32 i;

	mac_lo = omap_ctrl_readl(TI81XX_CONTROL_MAC_ID0_LO);
	mac_hi = omap_ctrl_readl(TI81XX_CONTROL_MAC_ID0_HI);
	am33xx_cpsw_slaves[0].mac_addr[0] = mac_hi & 0xFF;
	am33xx_cpsw_slaves[0].mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	am33xx_cpsw_slaves[0].mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
	am33xx_cpsw_slaves[0].mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
	am33xx_cpsw_slaves[0].mac_addr[4] = mac_lo & 0xFF;
	am33xx_cpsw_slaves[0].mac_addr[5] = (mac_lo & 0xFF00) >> 8;

	/* Read MACID0 from eeprom if eFuse MACID is invalid */
	if (!is_valid_ether_addr(am33xx_cpsw_slaves[0].mac_addr)) {
		for (i = 0; i < ETH_ALEN; i++)
			am33xx_cpsw_slaves[0].mac_addr[i] = am33xx_macid0[i];
	}

	mac_lo = omap_ctrl_readl(TI81XX_CONTROL_MAC_ID1_LO);
	mac_hi = omap_ctrl_readl(TI81XX_CONTROL_MAC_ID1_HI);
	am33xx_cpsw_slaves[1].mac_addr[0] = mac_hi & 0xFF;
	am33xx_cpsw_slaves[1].mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	am33xx_cpsw_slaves[1].mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
	am33xx_cpsw_slaves[1].mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
	am33xx_cpsw_slaves[1].mac_addr[4] = mac_lo & 0xFF;
	am33xx_cpsw_slaves[1].mac_addr[5] = (mac_lo & 0xFF00) >> 8;

	/* Read MACID1 from eeprom if eFuse MACID is invalid */
	if (!is_valid_ether_addr(am33xx_cpsw_slaves[1].mac_addr)) {
		for (i = 0; i < ETH_ALEN; i++)
			am33xx_cpsw_slaves[1].mac_addr[i] = am33xx_macid1[i];
	}

	if (am33xx_evmid == IND_AUT_MTR_EVM) {
		am33xx_cpsw_slaves[0].phy_id = "0:1e";
		am33xx_cpsw_slaves[1].phy_id = "0:00";
	}

	memcpy(am33xx_cpsw_pdata.mac_addr,
			am33xx_cpsw_slaves[0].mac_addr, ETH_ALEN);
	platform_device_register(&am33xx_cpsw_mdiodevice);
	platform_device_register(&am33xx_cpsw_device);
	clk_add_alias(NULL, dev_name(&am33xx_cpsw_mdiodevice.dev),
			NULL, &am33xx_cpsw_device.dev);
}


#if defined(CONFIG_OMAP_WATCHDOG) || defined(CONFIG_OMAP_WATCHDOG_MODULE)
static int __init omap_init_wdt(void)
{
	int id = -1;
	struct platform_device *pdev;
	struct omap_hwmod *oh;
	char *oh_name = "wd_timer2";
	char *dev_name = "omap_wdt";

	if (!cpu_class_is_omap2())
		return 0;

	oh = omap_hwmod_lookup(oh_name);
	if (!oh) {
		pr_err("Could not look up wd_timer%d hwmod\n", id);
		return -EINVAL;
	}

	pdev = omap_device_build(dev_name, id, oh, NULL, 0, NULL, 0, 0);
	WARN(IS_ERR(pdev), "Can't build omap_device for %s:%s.\n",
				dev_name, oh->name);
	return 0;
}
subsys_initcall(omap_init_wdt);
#endif
