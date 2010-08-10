/*
 * ASoC Audio Layer for Freescale MXS ADC/DAC
 *
 * Author: Vladislav Buzov <vbuzov@embeddedalley.com>
 *
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <mach/dma.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/regs-audioin.h>
#include <mach/regs-audioout.h>

#include "mxs-pcm.h"

#define MXS_ADC_RATES	SNDRV_PCM_RATE_8000_192000
#define MXS_ADC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
				SNDRV_PCM_FMTBIT_S32_LE)

struct mxs_pcm_dma_params mxs_audio_in = {
	.name = "mxs-audio-in",
	.dma_ch	= MXS_DMA_CHANNEL_AHB_APBX_AUDIOADC,
	.irq = IRQ_ADC_DMA,
};

struct mxs_pcm_dma_params mxs_audio_out = {
	.name = "mxs-audio-out",
	.dma_ch	= MXS_DMA_CHANNEL_AHB_APBX_AUDIODAC,
	.irq = IRQ_DAC_DMA,
};

static irqreturn_t mxs_err_irq(int irq, void *dev_id)
{
	struct snd_pcm_substream *substream = dev_id;
	int playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? 1 : 0;
	u32 ctrl_reg;
	u32 overflow_mask;
	u32 underflow_mask;

	if (playback) {
		ctrl_reg = __raw_readl(REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL);
		underflow_mask = BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ;
		overflow_mask = BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ;
	} else {
		ctrl_reg = __raw_readl(REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL);
		underflow_mask = BM_AUDIOIN_CTRL_FIFO_UNDERFLOW_IRQ;
		overflow_mask = BM_AUDIOIN_CTRL_FIFO_OVERFLOW_IRQ;
	}

	if (ctrl_reg & underflow_mask) {
		printk(KERN_DEBUG "%s underflow detected\n",
		       playback ? "DAC" : "ADC");

		if (playback)
			__raw_writel(
				BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		else
			__raw_writel(
				BM_AUDIOIN_CTRL_FIFO_UNDERFLOW_IRQ,
				REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);

	} else if (ctrl_reg & overflow_mask) {
		printk(KERN_DEBUG "%s overflow detected\n",
		       playback ? "DAC" : "ADC");

		if (playback)
			__raw_writel(
				BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		else
			__raw_writel(BM_AUDIOIN_CTRL_FIFO_OVERFLOW_IRQ,
				REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);
	} else
		printk(KERN_WARNING "Unknown DAC error interrupt\n");

	return IRQ_HANDLED;
}

static int mxs_adc_trigger(struct snd_pcm_substream *substream,
				int cmd,
				struct snd_soc_dai *dai)
{
	int playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? 1 : 0;
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (playback)
			__raw_writel(BM_AUDIOOUT_CTRL_RUN,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_SET);
		else
			__raw_writel(BM_AUDIOIN_CTRL_RUN,
				REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_SET);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
		if (playback)
			__raw_writel(BM_AUDIOOUT_CTRL_RUN,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		else
			__raw_writel(BM_AUDIOIN_CTRL_RUN,
				REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);
		break;

	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int mxs_adc_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? 1 : 0;
	int irq;
	int ret;

	if (playback) {
		irq = IRQ_DAC_ERROR;
		cpu_dai->dma_data = &mxs_audio_out;
	} else {
		irq = IRQ_ADC_ERROR;
		cpu_dai->dma_data = &mxs_audio_in;
	}

	ret = request_irq(irq, mxs_err_irq, 0, "MXS DAC/ADC Error",
			  substream);
	if (ret) {
		printk(KERN_ERR "%s: Unable to request ADC/DAC error irq %d\n",
		       __func__, IRQ_DAC_ERROR);
		return ret;
	}

	/* Enable error interrupt */
	if (playback) {
		__raw_writel(BM_AUDIOOUT_CTRL_FIFO_OVERFLOW_IRQ,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		__raw_writel(BM_AUDIOOUT_CTRL_FIFO_UNDERFLOW_IRQ,
				REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		__raw_writel(BM_AUDIOOUT_CTRL_FIFO_ERROR_IRQ_EN,
			REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_SET);
	} else {
		__raw_writel(BM_AUDIOIN_CTRL_FIFO_OVERFLOW_IRQ,
			REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);
		__raw_writel(BM_AUDIOIN_CTRL_FIFO_UNDERFLOW_IRQ,
			REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);
		__raw_writel(BM_AUDIOIN_CTRL_FIFO_ERROR_IRQ_EN,
			REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_SET);
	}

	return 0;
}

static void mxs_adc_shutdown(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	int playback = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? 1 : 0;

	/* Disable error interrupt */
	if (playback) {
		__raw_writel(BM_AUDIOOUT_CTRL_FIFO_ERROR_IRQ_EN,
			REGS_AUDIOOUT_BASE + HW_AUDIOOUT_CTRL_CLR);
		free_irq(IRQ_DAC_ERROR, substream);
	} else {
		__raw_writel(BM_AUDIOIN_CTRL_FIFO_ERROR_IRQ_EN,
			REGS_AUDIOIN_BASE + HW_AUDIOIN_CTRL_CLR);
		free_irq(IRQ_ADC_ERROR, substream);
	}
}

#ifdef CONFIG_PM
static int mxs_adc_suspend(struct snd_soc_dai *cpu_dai)
{
	return 0;
}

static int mxs_adc_resume(struct snd_soc_dai *cpu_dai)
{
	return 0;
}
#else
#define mxs_adc_suspend	NULL
#define mxs_adc_resume	NULL
#endif /* CONFIG_PM */

struct snd_soc_dai_ops mxs_adc_dai_ops = {
	.startup = mxs_adc_startup,
	.shutdown = mxs_adc_shutdown,
	.trigger = mxs_adc_trigger,
};

struct snd_soc_dai mxs_adc_dai = {
	.name = "mxs adc/dac",
	.id = 0,
	.suspend = mxs_adc_suspend,
	.resume = mxs_adc_resume,
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = MXS_ADC_RATES,
		.formats = MXS_ADC_FORMATS,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = MXS_ADC_RATES,
		.formats = MXS_ADC_FORMATS,
	},
	.ops = &mxs_adc_dai_ops,
};
EXPORT_SYMBOL_GPL(mxs_adc_dai);

static int __init mxs_adc_dai_init(void)
{
	return snd_soc_register_dai(&mxs_adc_dai);
}

static void __exit mxs_adc_dai_exit(void)
{
	snd_soc_unregister_dai(&mxs_adc_dai);
}
module_init(mxs_adc_dai_init);
module_exit(mxs_adc_dai_exit);

MODULE_AUTHOR("Vladislav Buzov");
MODULE_DESCRIPTION("MXS ADC/DAC DAI");
MODULE_LICENSE("GPL");
