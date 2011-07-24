/*
 * soc-pcm.c  --  ALSA SoC PCM
 *
 * Copyright 2005 Wolfson Microelectronics PLC.
 * Copyright 2005 Openedhand Ltd.
 * Copyright (C) 2010 Slimlogic Ltd.
 * Copyright (C) 2010 Texas Instruments Inc.
 *
 * Authors: Liam Girdwood <lrg@ti.com>
 *          Mark Brown <broonie@opensource.wolfsonmicro.com>       
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#undef DEBUG

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dsp.h>
#include <sound/initval.h>

static DEFINE_MUTEX(pcm_mutex);

static int soc_pcm_apply_symmetry(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;

	if (!codec_dai->driver->symmetric_rates &&
	    !cpu_dai->driver->symmetric_rates &&
	    !rtd->dai_link->symmetric_rates)
		return 0;

	/* This can happen if multiple streams are starting simultaneously -
	 * the second can need to get its constraints before the first has
	 * picked a rate.  Complain and allow the application to carry on.
	 */
	if (!rtd->rate) {
		dev_warn(&rtd->dev,
			 "Not enforcing symmetric_rates due to race\n");
		return 0;
	}

	dev_dbg(&rtd->dev, "Symmetry forces %dHz rate\n", rtd->rate);

	ret = snd_pcm_hw_constraint_minmax(substream->runtime,
					   SNDRV_PCM_HW_PARAM_RATE,
					   rtd->rate, rtd->rate);
	if (ret < 0) {
		dev_err(&rtd->dev,
			"Unable to apply rate symmetry constraint: %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * Called by ALSA when a PCM substream is opened, the runtime->hw record is
 * then initialized and any private data can be allocated. This also calls
 * startup for the cpu DAI, platform, machine and codec DAI.
 */
static int soc_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai_driver *cpu_dai_drv = cpu_dai->driver;
	struct snd_soc_dai_driver *codec_dai_drv = codec_dai->driver;
	int ret = 0;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	/* startup the audio subsystem */
	if (cpu_dai->driver->ops->startup) {
		ret = cpu_dai->driver->ops->startup(substream, cpu_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: can't open interface %s\n",
				cpu_dai->name);
			goto out;
		}
	}

	if (platform->driver->ops && platform->driver->ops->open) {
		ret = platform->driver->ops->open(substream);
		if (ret < 0) {
			printk(KERN_ERR "asoc: can't open platform %s\n", platform->name);
			goto platform_err;
		}
	}

	if (codec_dai->driver->ops->startup) {
		ret = codec_dai->driver->ops->startup(substream, codec_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: can't open codec %s\n",
				codec_dai->name);
			goto codec_dai_err;
		}
	}

	if (rtd->dai_link->ops && rtd->dai_link->ops->startup) {
		ret = rtd->dai_link->ops->startup(substream);
		if (ret < 0) {
			printk(KERN_ERR "asoc: %s startup failed\n", rtd->dai_link->name);
			goto machine_err;
		}
	}

	/* DSP DAI links compat checks are different */
	if (rtd->dai_link->dynamic || rtd->dai_link->no_pcm)
		goto dynamic;

	/* Check that the codec and cpu DAIs are compatible */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		runtime->hw.rate_min =
			max(codec_dai_drv->playback.rate_min,
			    cpu_dai_drv->playback.rate_min);
		runtime->hw.rate_max =
			min(codec_dai_drv->playback.rate_max,
			    cpu_dai_drv->playback.rate_max);
		runtime->hw.channels_min =
			max(codec_dai_drv->playback.channels_min,
				cpu_dai_drv->playback.channels_min);
		runtime->hw.channels_max =
			min(codec_dai_drv->playback.channels_max,
				cpu_dai_drv->playback.channels_max);
		runtime->hw.formats =
			codec_dai_drv->playback.formats & cpu_dai_drv->playback.formats;
		runtime->hw.rates =
			codec_dai_drv->playback.rates & cpu_dai_drv->playback.rates;
		if (codec_dai_drv->playback.rates
			   & (SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_CONTINUOUS))
			runtime->hw.rates |= cpu_dai_drv->playback.rates;
		if (cpu_dai_drv->playback.rates
			   & (SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_CONTINUOUS))
			runtime->hw.rates |= codec_dai_drv->playback.rates;
	} else {
		runtime->hw.rate_min =
			max(codec_dai_drv->capture.rate_min,
			    cpu_dai_drv->capture.rate_min);
		runtime->hw.rate_max =
			min(codec_dai_drv->capture.rate_max,
			    cpu_dai_drv->capture.rate_max);
		runtime->hw.channels_min =
			max(codec_dai_drv->capture.channels_min,
				cpu_dai_drv->capture.channels_min);
		runtime->hw.channels_max =
			min(codec_dai_drv->capture.channels_max,
				cpu_dai_drv->capture.channels_max);
		runtime->hw.formats =
			codec_dai_drv->capture.formats & cpu_dai_drv->capture.formats;
		runtime->hw.rates =
			codec_dai_drv->capture.rates & cpu_dai_drv->capture.rates;
		if (codec_dai_drv->capture.rates
			   & (SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_CONTINUOUS))
			runtime->hw.rates |= cpu_dai_drv->capture.rates;
		if (cpu_dai_drv->capture.rates
			   & (SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_CONTINUOUS))
			runtime->hw.rates |= codec_dai_drv->capture.rates;
	}

	ret = -EINVAL;
	snd_pcm_limit_hw_rates(runtime);
	if (!runtime->hw.rates) {
		printk(KERN_ERR "asoc: %s <-> %s No matching rates\n",
			codec_dai->name, cpu_dai->name);
		goto config_err;
	}
	if (!runtime->hw.formats) {
		printk(KERN_ERR "asoc: %s <-> %s No matching formats\n",
			codec_dai->name, cpu_dai->name);
		goto config_err;
	}
	if (!runtime->hw.channels_min || !runtime->hw.channels_max ||
	    runtime->hw.channels_min > runtime->hw.channels_max) {
		printk(KERN_ERR "asoc: %s <-> %s No matching channels\n",
				codec_dai->name, cpu_dai->name);
		goto config_err;
	}

	/* Symmetry only applies if we've already got an active stream. */
	if (cpu_dai->active || codec_dai->active) {
		ret = soc_pcm_apply_symmetry(substream);
		if (ret != 0)
			goto config_err;
	}

	pr_debug("asoc: %s <-> %s info:\n",
			codec_dai->name, cpu_dai->name);
	pr_debug("asoc: rate mask 0x%x\n", runtime->hw.rates);
	pr_debug("asoc: min ch %d max ch %d\n", runtime->hw.channels_min,
		 runtime->hw.channels_max);
	pr_debug("asoc: min rate %d max rate %d\n", runtime->hw.rate_min,
		 runtime->hw.rate_max);

dynamic:
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		cpu_dai->playback_active++;
		codec_dai->playback_active++;
	} else {
		cpu_dai->capture_active++;
		codec_dai->capture_active++;
	}
	cpu_dai->active++;
	codec_dai->active++;
	rtd->codec->active++;
	mutex_unlock(&rtd->pcm_mutex);
	return 0;

config_err:
	if (rtd->dai_link->ops && rtd->dai_link->ops->shutdown)
		rtd->dai_link->ops->shutdown(substream);

machine_err:
	if (codec_dai->driver->ops->shutdown)
		codec_dai->driver->ops->shutdown(substream, codec_dai);

codec_dai_err:
	if (platform->driver->ops && platform->driver->ops->close)
		platform->driver->ops->close(substream);

platform_err:
	if (cpu_dai->driver->ops->shutdown)
		cpu_dai->driver->ops->shutdown(substream, cpu_dai);
out:
	mutex_unlock(&rtd->pcm_mutex);
	return ret;
}

/*
 * Power down the audio subsystem pmdown_time msecs after close is called.
 * This is to ensure there are no pops or clicks in between any music tracks
 * due to DAPM power cycling.
 */
static void close_delayed_work(struct work_struct *work)
{
	struct snd_soc_pcm_runtime *rtd =
			container_of(work, struct snd_soc_pcm_runtime, delayed_work.work);
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	pr_debug("pop wq checking: %s status: %s waiting: %s\n",
		 codec_dai->driver->playback.stream_name,
		 codec_dai->playback_active ? "active" : "inactive",
		 codec_dai->pop_wait ? "yes" : "no");

	/* are we waiting on this codec DAI stream */
	if (codec_dai->pop_wait == 1) {
		codec_dai->pop_wait = 0;
		snd_soc_dapm_stream_event(rtd,
			codec_dai->driver->playback.stream_name,
			SND_SOC_DAPM_STREAM_STOP);
	}

	mutex_unlock(&rtd->pcm_mutex);
}

/*
 * Called by ALSA when a PCM substream is closed. Private data can be
 * freed here. The cpu DAI, codec DAI, machine and platform are also
 * shutdown.
 */
static int soc_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		cpu_dai->playback_active--;
		codec_dai->playback_active--;
	} else {
		cpu_dai->capture_active--;
		codec_dai->capture_active--;
	}

	cpu_dai->active--;
	codec_dai->active--;
	codec->active--;

	/* Muting the DAC suppresses artifacts caused during digital
	 * shutdown, for example from stopping clocks.
	 */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_digital_mute(codec_dai, 1);

	if (cpu_dai->driver->ops->shutdown)
		cpu_dai->driver->ops->shutdown(substream, cpu_dai);

	if (codec_dai->driver->ops->shutdown)
		codec_dai->driver->ops->shutdown(substream, codec_dai);

	if (rtd->dai_link->ops && rtd->dai_link->ops->shutdown)
		rtd->dai_link->ops->shutdown(substream);

	if (platform->driver->ops && platform->driver->ops->close)
		platform->driver->ops->close(substream);
	cpu_dai->runtime = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* start delayed pop wq here for playback streams */
		codec_dai->pop_wait = 1;
		schedule_delayed_work(&rtd->delayed_work,
			msecs_to_jiffies(rtd->pmdown_time));
	} else {
		/* capture streams can be powered down now */
		snd_soc_dapm_stream_event(rtd,
			codec_dai->driver->capture.stream_name,
			SND_SOC_DAPM_STREAM_STOP);
	}

	mutex_unlock(&rtd->pcm_mutex);
	return 0;
}

/*
 * Called by ALSA when the PCM substream is prepared, can set format, sample
 * rate, etc.  This function is non atomic and can be called multiple times,
 * it can refer to the runtime info.
 */
static int soc_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	if (rtd->dai_link->ops && rtd->dai_link->ops->prepare) {
		ret = rtd->dai_link->ops->prepare(substream);
		if (ret < 0) {
			printk(KERN_ERR "asoc: machine prepare error\n");
			goto out;
		}
	}

	if (platform->driver->ops && platform->driver->ops->prepare) {
		ret = platform->driver->ops->prepare(substream);
		if (ret < 0) {
			printk(KERN_ERR "asoc: platform prepare error\n");
			goto out;
		}
	}

	if (codec_dai->driver->ops->prepare) {
		ret = codec_dai->driver->ops->prepare(substream, codec_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: codec DAI prepare error\n");
			goto out;
		}
	}

	if (cpu_dai->driver->ops->prepare) {
		ret = cpu_dai->driver->ops->prepare(substream, cpu_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: cpu DAI prepare error\n");
			goto out;
		}
	}

	/* cancel any delayed stream shutdown that is pending */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK &&
	    codec_dai->pop_wait) {
		codec_dai->pop_wait = 0;
		cancel_delayed_work(&rtd->delayed_work);
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dapm_stream_event(rtd,
					  codec_dai->driver->playback.stream_name,
					  SND_SOC_DAPM_STREAM_START);
	else
		snd_soc_dapm_stream_event(rtd,
					  codec_dai->driver->capture.stream_name,
					  SND_SOC_DAPM_STREAM_START);

	snd_soc_dai_digital_mute(codec_dai, 0);

out:
	mutex_unlock(&rtd->pcm_mutex);
	return ret;
}

/*
 * Called by ALSA when the hardware params are set by application. This
 * function can also be called multiple times and can allocate buffers
 * (using snd_pcm_lib_* ). It's non-atomic.
 */
static int soc_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	if (rtd->dai_link->ops && rtd->dai_link->ops->hw_params) {
		ret = rtd->dai_link->ops->hw_params(substream, params);
		if (ret < 0) {
			printk(KERN_ERR "asoc: machine hw_params failed\n");
			goto out;
		}
	}

	if (codec_dai->driver->ops->hw_params) {
		ret = codec_dai->driver->ops->hw_params(substream, params, codec_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: can't set codec %s hw params\n",
				codec_dai->name);
			goto codec_err;
		}
	}

	if (cpu_dai->driver->ops->hw_params) {
		ret = cpu_dai->driver->ops->hw_params(substream, params, cpu_dai);
		if (ret < 0) {
			printk(KERN_ERR "asoc: interface %s hw params failed\n",
				cpu_dai->name);
			goto interface_err;
		}
	}

	if (platform->driver->ops && platform->driver->ops->hw_params) {
		ret = platform->driver->ops->hw_params(substream, params);
		if (ret < 0) {
			printk(KERN_ERR "asoc: platform %s hw params failed\n",
				platform->name);
			goto platform_err;
		}
	}

	rtd->rate = params_rate(params);

out:
	mutex_unlock(&rtd->pcm_mutex);
	return ret;

platform_err:
	if (cpu_dai->driver->ops->hw_free)
		cpu_dai->driver->ops->hw_free(substream, cpu_dai);

interface_err:
	if (codec_dai->driver->ops->hw_free)
		codec_dai->driver->ops->hw_free(substream, codec_dai);

codec_err:
	if (rtd->dai_link->ops && rtd->dai_link->ops->hw_free)
		rtd->dai_link->ops->hw_free(substream);

	mutex_unlock(&rtd->pcm_mutex);
	return ret;
}

/*
 * Frees resources allocated by hw_params, can be called multiple times
 */
static int soc_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec = rtd->codec;

	mutex_lock_nested(&rtd->pcm_mutex, rtd->pcm_subclass);

	/* apply codec digital mute */
	if (!codec->active)
		snd_soc_dai_digital_mute(codec_dai, 1);

	/* free any machine hw params */
	if (rtd->dai_link->ops && rtd->dai_link->ops->hw_free)
		rtd->dai_link->ops->hw_free(substream);

	/* free any DMA resources */
	if (platform->driver->ops && platform->driver->ops->hw_free)
		platform->driver->ops->hw_free(substream);

	/* now free hw params for the DAIs  */
	if (codec_dai->driver->ops->hw_free)
		codec_dai->driver->ops->hw_free(substream, codec_dai);

	if (cpu_dai->driver->ops->hw_free)
		cpu_dai->driver->ops->hw_free(substream, cpu_dai);

	mutex_unlock(&rtd->pcm_mutex);
	return 0;
}

static int soc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;

	if (codec_dai->driver->ops->trigger) {
		ret = codec_dai->driver->ops->trigger(substream, cmd, codec_dai);
		if (ret < 0)
			return ret;
	}

	if (platform->driver->ops && platform->driver->ops->trigger) {
		ret = platform->driver->ops->trigger(substream, cmd);
		if (ret < 0)
			return ret;
	}

	if (cpu_dai->driver->ops->trigger) {
		ret = cpu_dai->driver->ops->trigger(substream, cmd, cpu_dai);
		if (ret < 0)
			return ret;
	}
	return 0;
}

/*
 * soc level wrapper for pointer callback
 * If cpu_dai, codec_dai, platform driver has the delay callback, than
 * the runtime->delay will be updated accordingly.
 */
static snd_pcm_uframes_t soc_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_uframes_t offset = 0;
	snd_pcm_sframes_t delay = 0;

	if (platform->driver->ops && platform->driver->ops->pointer)
		offset = platform->driver->ops->pointer(substream);

	if (cpu_dai->driver->ops->delay)
		delay += cpu_dai->driver->ops->delay(substream, cpu_dai);

	if (codec_dai->driver->ops->delay)
		delay += codec_dai->driver->ops->delay(substream, codec_dai);

	if (platform->driver->delay)
		delay += platform->driver->delay(substream, codec_dai);

	runtime->delay = delay;

	return offset;
}

static inline int be_connect(struct snd_soc_pcm_runtime *fe,
		struct snd_soc_pcm_runtime *be, int stream)
{
	struct snd_soc_dsp_params *dsp_params;

	/* only add new dsp_paramss */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {
		if (dsp_params->be == be && dsp_params->fe == fe)
			return 0;
	}

	dsp_params = kzalloc(sizeof(struct snd_soc_dsp_params), GFP_KERNEL);
	if (!dsp_params)
		return -ENOMEM;

	dsp_params->be = be;
	dsp_params->fe = fe;
	be->dsp[stream].runtime = fe->dsp[stream].runtime;
	dsp_params->state = SND_SOC_DSP_LINK_STATE_NEW;
	list_add(&dsp_params->list_be, &fe->dsp[stream].be_clients);
	list_add(&dsp_params->list_fe, &be->dsp[stream].fe_clients);

	dev_dbg(&fe->dev, "  connected new DSP %s path %s %s %s\n",
			stream ? "capture" : "playback",  fe->dai_link->name,
			stream ? "<-" : "->", be->dai_link->name);

#ifdef CONFIG_DEBUG_FS
	dsp_params->debugfs_state = debugfs_create_u32(be->dai_link->name, 0644,
			fe->debugfs_dsp_root, &dsp_params->state);
#endif

	return 1;
}

static inline void be_disconnect(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params, *d;

	list_for_each_entry_safe(dsp_params, d, &fe->dsp[stream].be_clients, list_be) {
		if (dsp_params->state == SND_SOC_DSP_LINK_STATE_FREE) {
			dev_dbg(&fe->dev, "  freed DSP %s path %s %s %s\n",
					stream ? "capture" : "playback", fe->dai_link->name,
					stream ? "<-" : "->", dsp_params->be->dai_link->name);

#ifdef CONFIG_DEBUG_FS
			debugfs_remove(dsp_params->debugfs_state);
#endif

			list_del(&dsp_params->list_be);
			list_del(&dsp_params->list_fe);
			kfree(dsp_params);
		}
	}
}

static struct snd_soc_pcm_runtime *be_get_rtd(struct snd_soc_card *card,
		struct snd_soc_dapm_widget *widget)
{
	struct snd_soc_pcm_runtime *be;
	int i;

	if (!widget->sname)
		return NULL;

	for (i = 0; i < card->num_links; i++) {
		be = &card->rtd[i];

		if (!strcmp(widget->sname, be->dai_link->stream_name))
			return be;
	}

	return NULL;
}

static struct snd_soc_dapm_widget *be_get_widget(struct snd_soc_card *card,
		struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dapm_widget *widget;

	list_for_each_entry(widget, &card->widgets, list) {

		if (!widget->sname)
			continue;

		if (!strcmp(widget->sname, rtd->dai_link->stream_name))
			return widget;
	}

	return NULL;
}

static int widget_in_list(struct snd_soc_dapm_widget_list *list,
		struct snd_soc_dapm_widget *widget)
{
	int i;

	for (i = 0; i < list->num_widgets; i++) {
			if (widget == list->widgets[i])
				return 1;
	}

	return 0;
}

/*
 * Find the corresponding BE DAIs that source or sink audio to this
 * FE substream.
 */
static int dsp_add_new_paths(struct snd_soc_pcm_runtime *fe,
	int stream, int pending)
{
	struct snd_soc_dai *cpu_dai = fe->cpu_dai;
	struct snd_soc_card *card = fe->card;
	struct snd_soc_dapm_widget_list *list;
	enum snd_soc_dapm_type fe_type, be_type;
	int i, count = 0, err, paths;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		fe_type = snd_soc_dapm_aif_in;
		be_type = snd_soc_dapm_aif_out;
	} else {
		fe_type = snd_soc_dapm_aif_out;
		be_type = snd_soc_dapm_aif_in;
	}

	/* get number of valid playback paths and their widgets */
	paths = snd_soc_dapm_get_connected_widgets_type(&card->dapm,
			cpu_dai->driver->name, &list, stream, fe_type);

	dev_dbg(&fe->dev, "found %d audio %s paths\n", paths,
			stream ? "capture" : "playback");
	if (!paths)
		goto out;

	/* find BE DAI widgets and and connect the to FE */
	for (i = 0; i < list->num_widgets; i++) {

		if (list->widgets[i]->id == be_type) {
			struct snd_soc_pcm_runtime *be;

			/* is there a valid BE rtd for this widget */
			be = be_get_rtd(card, list->widgets[i]);
			if (!be) {
				dev_err(&fe->dev, "no BE found for %s\n",
						list->widgets[i]->name);
				continue;
			}

			/* newly connected FE and BE */
			err = be_connect(fe, be, stream);
			if (err < 0) {
				dev_err(&fe->dev, "can't connect %s\n", list->widgets[i]->name);
				break;
			} else if (err == 0)
				continue;

			be->dsp[stream].runtime_update = pending;
			count++;
		}
	}

out:
	kfree(list);
	return count;
}

/*
 * Find the corresponding BE DAIs that source or sink audio to this
 * FE substream.
 */
static int dsp_prune_old_paths(struct snd_soc_pcm_runtime *fe, int stream,
	int pending)
{
	struct snd_soc_dai *cpu_dai = fe->cpu_dai;
	struct snd_soc_card *card = fe->card;
	struct snd_soc_dsp_params *dsp_params;
	struct snd_soc_dapm_widget_list *list;
	int count = 0, paths;
	enum snd_soc_dapm_type fe_type, be_type;
	struct snd_soc_dapm_widget *widget;

	dev_dbg(&fe->dev, "scan for old %s %s streams\n", fe->dai_link->name,
			stream ? "capture" : "playback");

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		fe_type = snd_soc_dapm_aif_in;
		be_type = snd_soc_dapm_aif_out;
	} else {
		fe_type = snd_soc_dapm_aif_out;
		be_type = snd_soc_dapm_aif_in;
	}

	/* get number of valid playback paths and their widgets */
	paths = snd_soc_dapm_get_connected_widgets_type(&card->dapm,
			cpu_dai->driver->name, &list, stream, fe_type);

	dev_dbg(&fe->dev, "found %d audio %s paths\n", paths,
			stream ? "capture" : "playback");
	if (!paths) {
		/* prune all BEs */
		list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

			dsp_params->state = SND_SOC_DSP_LINK_STATE_FREE;
			dsp_params->be->dsp[stream].runtime_update = pending;
			count++;
		}

		dev_dbg(&fe->dev, "pruned all %s BE for FE %s\n", fe->dai_link->name,
			stream ? "capture" : "playback");
		goto out;
	}

	/* search card for valid BE AIFs */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		/* is there a valid widget for this BE */
		widget = be_get_widget(card, dsp_params->be);
		if (!widget) {
			dev_err(&fe->dev, "no widget found for %s\n",
					dsp_params->be->dai_link->name);
			continue;
		}

		/* prune the BE if it's no longer in our active list */
		if (widget_in_list(list, widget))
			continue;

		dev_dbg(&fe->dev, "pruning %s BE %s for %s\n",
			stream ? "capture" : "playback", dsp_params->be->dai_link->name,
			fe->dai_link->name);
		dsp_params->state = SND_SOC_DSP_LINK_STATE_FREE;
		dsp_params->be->dsp[stream].runtime_update = pending;
		count++;
	}

	/* the number of old paths pruned */
out:
	kfree(list);
	return count;
}

/*
 * Update the state of all BE's with state old to state new.
 */
static void be_state_update(struct snd_soc_pcm_runtime *fe, int stream,
		enum snd_soc_dsp_link_state old, enum snd_soc_dsp_link_state new)
{
	struct snd_soc_dsp_params *dsp_params;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {
		if (dsp_params->state == old)
			dsp_params->state = new;
	}
}

/*
 * Update the state of all BE's to new regardless of current state.
 */
static void fe_state_update(struct snd_soc_pcm_runtime *fe, int stream,
		enum snd_soc_dsp_link_state new)
{
	struct snd_soc_dsp_params *dsp_params;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be)
			dsp_params->state = new;
}

/*
 * Clear the runtime pending state of all BE's.
 */
static void fe_clear_pending(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be)
			dsp_params->be->dsp[stream].runtime_update = 0;
}

/* Unwind the BE startup */
static void soc_dsp_be_dai_startup_unwind(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;

	/* disable any enabled and non active backends */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		if (--dsp_params->be->dsp[stream].users != 0)
			continue;

		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_NEW)
			continue;

		soc_pcm_close(be_substream);
		be_substream->runtime = NULL;
	}
}

/* Startup all new BE */
static int soc_dsp_be_dai_startup(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;
	int err, count = 0;

	/* only startup BE DAIs that are either sinks or sources to this FE DAI */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		/* first time the dsp_params is open ? */
		if (dsp_params->be->dsp[stream].users++ != 0)
			continue;

		/* only open and ref count new links */
		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_NEW)
			continue;

		dev_dbg(&dsp_params->be->dev, "dsp: open BE %s\n",
				dsp_params->be->dai_link->name);

		be_substream->runtime = dsp_params->be->dsp[stream].runtime;
		err = soc_pcm_open(be_substream);
		if (err < 0)
			goto unwind;
		count++;
	}

	/* update BE state */
	be_state_update(fe, stream,
			SND_SOC_DSP_LINK_STATE_NEW, SND_SOC_DSP_LINK_STATE_HW_PARAMS);
	return count;

unwind:
	/* disable any enabled and non active backends */
	list_for_each_entry_continue_reverse(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		if (--dsp_params->be->dsp[stream].users != 0)
			continue;

		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_NEW)
			continue;

		soc_pcm_close(be_substream);
		be_substream->runtime = NULL;
	}

	/* update BE state for disconnect */
	be_state_update(fe, stream,
			SND_SOC_DSP_LINK_STATE_NEW, SND_SOC_DSP_LINK_STATE_FREE);
	return err;
}

void soc_dsp_set_dynamic_runtime(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai_driver *cpu_dai_drv = cpu_dai->driver;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		runtime->hw.rate_min = cpu_dai_drv->playback.rate_min;
		runtime->hw.rate_max = cpu_dai_drv->playback.rate_max;
		runtime->hw.channels_min = cpu_dai_drv->playback.channels_min;
		runtime->hw.channels_max = cpu_dai_drv->playback.channels_max;
		runtime->hw.formats &= cpu_dai_drv->playback.formats;
		runtime->hw.rates = cpu_dai_drv->playback.rates;
	} else {
		runtime->hw.rate_min = cpu_dai_drv->capture.rate_min;
		runtime->hw.rate_max = cpu_dai_drv->capture.rate_max;
		runtime->hw.channels_min = cpu_dai_drv->capture.channels_min;
		runtime->hw.channels_max = cpu_dai_drv->capture.channels_max;
		runtime->hw.formats &= cpu_dai_drv->capture.formats;
		runtime->hw.rates = cpu_dai_drv->capture.rates;
	}
}

static int soc_dsp_fe_dai_startup(struct snd_pcm_substream *fe_substream)
{
	struct snd_soc_pcm_runtime *fe = fe_substream->private_data;
	struct snd_pcm_runtime *runtime = fe_substream->runtime;
	int ret = 0;

	mutex_lock(&fe->card->dsp_mutex);

	ret = soc_dsp_be_dai_startup(fe, fe_substream->stream);
	if (ret < 0)
		goto be_err;

	dev_dbg(&fe->dev, "dsp: open FE %s\n", fe->dai_link->name);

	/* start the DAI frontend */
	ret = soc_pcm_open(fe_substream);
	if (ret < 0) {
		dev_err(&fe->dev,"dsp: failed to start FE %d\n", ret);
		goto unwind;
	}

	soc_dsp_set_dynamic_runtime(fe_substream);
	snd_pcm_limit_hw_rates(runtime);

	mutex_unlock(&fe->card->dsp_mutex);
	return 0;

unwind:
	soc_dsp_be_dai_startup_unwind(fe, fe_substream->stream);
be_err:
	mutex_unlock(&fe->card->dsp_mutex);
	return ret;
}

/* BE shutdown - called on DAPM sync updates (i.e. FE is already running)*/
static int soc_dsp_be_dai_shutdown(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;

	/* only shutdown backends that are either sinks or sources to this frontend DAI */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		if (--dsp_params->be->dsp[stream].users != 0)
			continue;

		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_FREE)
			continue;

		dev_dbg(&dsp_params->be->dev, "dsp: close BE %s\n",
			dsp_params->fe->dai_link->name);

		soc_pcm_close(be_substream);
		be_substream->runtime = NULL;
	}
	return 0;
}

/* FE +BE shutdown - called on FE PCM ops */
static int soc_dsp_fe_dai_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream;

	mutex_lock(&fe->card->dsp_mutex);

	/* shutdown the BEs */
	soc_dsp_be_dai_shutdown(fe, substream->stream);

	dev_dbg(&fe->dev, "dsp: close FE %s\n", fe->dai_link->name);

	/* now shutdown the frontend */
	soc_pcm_close(substream);

	/* run the stream event for each BE */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->playback.stream_name,
				SND_SOC_DAPM_STREAM_STOP);
	else
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->capture.stream_name,
				SND_SOC_DAPM_STREAM_STOP);

	mutex_unlock(&fe->card->dsp_mutex);
	return 0;
}

static int soc_dsp_be_dai_hw_params(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;
	int ret;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_HW_PARAMS)
			continue;

		/* first time the dsp_params is open ? */
		if (dsp_params->be->dsp[stream].users != 1)
			continue;

		dev_dbg(&dsp_params->be->dev, "dsp: hw_params BE %s\n",
			dsp_params->fe->dai_link->name);

		/* copy params for each dsp_params */
		memcpy(&dsp_params->params, &fe->dsp[stream].params,
				sizeof(struct snd_pcm_hw_params));

		/* perform any hw_params fixups */
		if (dsp_params->be->dai_link->be_hw_params_fixup) {
			ret = dsp_params->be->dai_link->be_hw_params_fixup(dsp_params->be,
					&dsp_params->params);
			if (ret < 0) {
				dev_err(&dsp_params->be->dev,
						"dsp: hw_params BE fixup failed %d\n", ret);
				return ret;
			}
		}

		ret = soc_pcm_hw_params(be_substream, &dsp_params->params);
		if (ret < 0) {
			dev_err(&dsp_params->be->dev, "dsp: hw_params BE failed %d\n", ret);
			return ret;
		}
	}
	return 0;
}

int soc_dsp_fe_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int ret;

	mutex_lock(&fe->card->dsp_mutex);

	memcpy(&fe->dsp[substream->stream].params, params,
			sizeof(struct snd_pcm_hw_params));
	ret = soc_dsp_be_dai_hw_params(fe, substream->stream);
	if (ret < 0)
		goto out;

	dev_dbg(&fe->dev, "dsp: hw_params FE %s\n", fe->dai_link->name);

	/* call hw_params on the frontend */
	ret = soc_pcm_hw_params(substream, params);
	if (ret < 0)
		dev_err(&fe->dev,"dsp: hw_params FE failed %d\n", ret);

out:
	mutex_unlock(&fe->card->dsp_mutex);
	return ret;
}

static int dsp_do_trigger(struct snd_soc_dsp_params *dsp_params,
		struct snd_pcm_substream *substream, int cmd)
{
	int ret;

	dev_dbg(&dsp_params->be->dev, "dsp: trigger BE %s cmd %d\n",
			dsp_params->fe->dai_link->name, cmd);

	ret = soc_pcm_trigger(substream, cmd);
	if (ret < 0)
		dev_err(&dsp_params->be->dev,"dsp: trigger BE failed %d\n", ret);

	return ret;
}

int soc_dsp_be_dai_trigger(struct snd_soc_pcm_runtime *fe, int stream, int cmd)
{
	struct snd_soc_dsp_params *dsp_params;
	int ret = 0;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			/* only start BEs that are not triggered */
			if (dsp_params->state == SND_SOC_DSP_LINK_STATE_PREPARE) {
				ret = dsp_do_trigger(dsp_params, be_substream, cmd);
				if (ret == 0)
					dsp_params->state = SND_SOC_DSP_LINK_STATE_START;
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			/* only stop BEs that are being shutdown */
			if (dsp_params->state == SND_SOC_DSP_LINK_STATE_FREE &&
				dsp_params->be->dsp[stream].users == 1)
				ret = dsp_do_trigger(dsp_params, be_substream, cmd);
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_RESUME:
			/* suspend and resume all BEs */
			ret = dsp_do_trigger(dsp_params, be_substream, cmd);
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/* only release Paused BEs */
			if (dsp_params->state == SND_SOC_DSP_LINK_STATE_PAUSED) {
				ret = dsp_do_trigger(dsp_params, be_substream, cmd);
				if (ret == 0)
					dsp_params->state = SND_SOC_DSP_LINK_STATE_START;
			}
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			/* only pause active BEs */
			if (dsp_params->state == SND_SOC_DSP_LINK_STATE_START) {
				ret = dsp_do_trigger(dsp_params, be_substream, cmd);
				if (ret == 0)
					dsp_params->state = SND_SOC_DSP_LINK_STATE_PAUSED;
			}
			break;
		}
		if (ret < 0)
			return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(soc_dsp_be_dai_trigger);

int soc_dsp_fe_dai_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	struct snd_soc_dsp_link *dsp_link = fe->dai_link->dsp_link;
	int stream = substream->stream, ret;

	switch (dsp_link->trigger[stream]) {
	case SND_SOC_DSP_TRIGGER_PRE:
		/* call trigger on the frontend before the backend. */

		dev_dbg(&fe->dev, "dsp: pre trigger FE %s cmd %d\n",
				fe->dai_link->name, cmd);

		ret = soc_pcm_trigger(substream, cmd);
		if (ret < 0) {
			dev_err(&fe->dev,"dsp: trigger FE failed %d\n", ret);
			return ret;
		}

		ret = soc_dsp_be_dai_trigger(fe, substream->stream, cmd);
		break;
	case SND_SOC_DSP_TRIGGER_POST:
		/* call trigger on the frontend after the backend. */

		ret = soc_dsp_be_dai_trigger(fe, substream->stream, cmd);
		if (ret < 0) {
			dev_err(&fe->dev,"dsp: trigger FE failed %d\n", ret);
			return ret;
		}

		dev_dbg(&fe->dev, "dsp: post trigger FE %s cmd %d\n",
				fe->dai_link->name, cmd);

		ret = soc_pcm_trigger(substream, cmd);
		break;
	case SND_SOC_DSP_TRIGGER_BESPOKE:
		/* bespoke trigger() - handles both FE and BEs */

		dev_dbg(&fe->dev, "dsp: bespoke trigger FE %s cmd %d\n",
				fe->dai_link->name, cmd);

		ret = soc_pcm_bespoke_trigger(substream, cmd);
		if (ret < 0) {
			dev_err(&fe->dev,"dsp: trigger FE failed %d\n", ret);
			return ret;
		}
		break;
	default:
		dev_err(&fe->dev, "dsp: invalid trigger cmd %d for %s\n", cmd,
				fe->dai_link->name);
		return -EINVAL;
	}

	return ret;
}

static int soc_dsp_be_dai_prepare(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;
	int ret = 0;

	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		/* only prepare ACTIVE or READY BE's */
		if (dsp_params->state == SND_SOC_DSP_LINK_STATE_NEW ||
				dsp_params->state == SND_SOC_DSP_LINK_STATE_FREE)
			continue;

		dev_dbg(&dsp_params->be->dev, "dsp: prepare BE %s\n",
			dsp_params->fe->dai_link->name);

		ret = soc_pcm_prepare(be_substream);
		if (ret < 0) {
			dev_err(&dsp_params->be->dev,"dsp: backend prepare failed %d\n",
					ret);
			break;
		}

		/* mark the BE as active */
		be_state_update(fe, stream, SND_SOC_DSP_LINK_STATE_HW_PARAMS,
				SND_SOC_DSP_LINK_STATE_PREPARE);
	}
	return ret;
}

int soc_dsp_fe_dai_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream, ret = 0;

	mutex_lock(&fe->card->dsp_mutex);

	dev_dbg(&fe->dev, "dsp: prepare FE %s\n", fe->dai_link->name);

	/* there is no point preparing this FE if there are no BEs */
	if (list_empty(&fe->dsp[stream].be_clients)) {
		dev_err(&fe->dev, "dsp: no backend DAIs enabled for %s\n",
				fe->dai_link->name);
		ret = -EINVAL;
		goto out;
	}

	ret = soc_dsp_be_dai_prepare(fe, substream->stream);
	if (ret < 0)
		goto out;

	/* mark the BE as active */
	fe_state_update(fe, stream, SND_SOC_DSP_LINK_STATE_PREPARE);

	/* call prepare on the frontend */
	ret = soc_pcm_prepare(substream);
	if (ret < 0) {
		dev_err(&fe->dev,"dsp: prepare FE %s failed\n", fe->dai_link->name);
		goto out;
	}

	/* run the stream event for each BE */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->playback.stream_name,
				SNDRV_PCM_TRIGGER_START);
	else
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->capture.stream_name,
				SNDRV_PCM_TRIGGER_START);

out:
	mutex_unlock(&fe->card->dsp_mutex);
	return ret;
}

static int soc_dsp_be_dai_hw_free(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_params;

	/* only hw_params backends that are either sinks or sources
	 * to this frontend DAI */
	list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

		struct snd_pcm_substream *be_substream =
			snd_soc_dsp_get_substream(dsp_params->be, stream);

		/* is this op for this BE ? */
		if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
			continue;

		if (dsp_params->state != SND_SOC_DSP_LINK_STATE_FREE)
			continue;

		/* only free hw when no longer used */
		if (dsp_params->be->dsp[stream].users != 1)
			continue;

		dev_dbg(&dsp_params->be->dev, "dsp: hw_free BE %s\n",
			dsp_params->fe->dai_link->name);

		soc_pcm_hw_free(be_substream);
	}

	return 0;
}

int soc_dsp_fe_dai_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int ret, stream = substream->stream;

	mutex_lock(&fe->card->dsp_mutex);

	fe_state_update(fe, stream, SND_SOC_DSP_LINK_STATE_FREE);

	dev_dbg(&fe->dev, "dsp: hw_free FE %s\n", fe->dai_link->name);

	/* call hw_free on the frontend */
	ret = soc_pcm_hw_free(substream);
	if (ret < 0)
		dev_err(&fe->dev,"dsp: hw_free FE %s failed\n", fe->dai_link->name);

	/* only hw_params backends that are either sinks or sources
	 * to this frontend DAI */
	ret = soc_dsp_be_dai_hw_free(fe, stream);

	mutex_unlock(&fe->card->dsp_mutex);
	return ret;
}

static int soc_pcm_ioctl(struct snd_pcm_substream *substream,
		     unsigned int cmd, void *arg)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;

	if (platform->driver->ops->ioctl)
		return platform->driver->ops->ioctl(substream, cmd, arg);
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

/*
 * FE stream event, send event to all active BEs.
 */
int soc_dsp_dapm_stream_event(struct snd_soc_pcm_runtime *fe,
	int dir, const char *stream, int event)
{
	struct snd_soc_dsp_params *dsp_params;

	/* resume for playback */
	list_for_each_entry(dsp_params, &fe->dsp[dir].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;

		dev_dbg(&be->dev, "pm: BE %s stream %s event %d dir %d\n",
				be->dai_link->name, stream, event, dir);

		snd_soc_dapm_stream_event(be, stream, event);
	}

	return 0;
}

static int dsp_run_update_shutdown(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_link *dsp_link = fe->dai_link->dsp_link;
	struct snd_pcm_substream *substream = snd_soc_dsp_get_substream(fe, stream);
	struct snd_soc_dsp_params *dsp_params;
	int ret;

	dev_dbg(&fe->dev, "runtime %s close on FE %s\n",
			stream ? "capture" : "playback", fe->dai_link->name);

	if (dsp_link->trigger[stream] == SND_SOC_DSP_TRIGGER_BESPOKE) {
		/* call bespoke trigger - FE takes care of all BE triggers */
		dev_dbg(&fe->dev, "dsp: bespoke trigger FE %s cmd stop\n",
				fe->dai_link->name);

		ret = soc_pcm_bespoke_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
		if (ret < 0) {
			dev_err(&fe->dev,"dsp: trigger FE failed %d\n", ret);
			return ret;
		}
	} else {

		list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

			dev_dbg(&fe->dev, "dsp: trigger FE %s cmd stop\n",
				fe->dai_link->name);

			ret = soc_dsp_be_dai_trigger(fe, stream, SNDRV_PCM_TRIGGER_STOP);
			if (ret < 0)
				return ret;
		}
	}

	ret = soc_dsp_be_dai_hw_free(fe, stream);
	if (ret < 0)
		return ret;

	ret = soc_dsp_be_dai_shutdown(fe, stream);
	if (ret < 0)
		return ret;

	/* run the stream event for each BE */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->playback.stream_name,
				SNDRV_PCM_TRIGGER_STOP);
	else
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->capture.stream_name,
				SNDRV_PCM_TRIGGER_STOP);

	return 0;
}

/* check for running FEs */
static int dsp_get_be_trigger_cmd(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_be_params, *dsp_fe_params;

	/* get the FEs for each BE */
	list_for_each_entry(dsp_be_params, &fe->dsp[stream].be_clients, list_be) {
		struct snd_soc_pcm_runtime *be = dsp_be_params->be;

		/* get the FEs for this BE */
		list_for_each_entry(dsp_fe_params, &be->dsp[stream].fe_clients, list_fe) {

			if (dsp_fe_params->state == SND_SOC_DSP_LINK_STATE_START)
				return SND_SOC_DSP_LINK_STATE_START;
		}
	}
	return SND_SOC_DSP_LINK_STATE_PAUSED;
}

/* check for running BEs */
static int dsp_get_fe_trigger_cmd(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_params *dsp_be_params;

	/* get the FEs for each BE */
	list_for_each_entry(dsp_be_params, &fe->dsp[stream].be_clients, list_be) {

		if (dsp_be_params->state == SND_SOC_DSP_LINK_STATE_START)
				return SND_SOC_DSP_LINK_STATE_START;
	}
	return SND_SOC_DSP_LINK_STATE_PAUSED;
}

static int dsp_run_update_startup(struct snd_soc_pcm_runtime *fe, int stream)
{
	struct snd_soc_dsp_link *dsp_link = fe->dai_link->dsp_link;
	struct snd_pcm_substream *substream = snd_soc_dsp_get_substream(fe, stream);
	struct snd_soc_dsp_params *dsp_params;
	int ret, cmd;

	dev_dbg(&fe->dev, "runtime %s open on FE %s\n",
			stream ? "capture" : "playback", fe->dai_link->name);

	ret = soc_dsp_be_dai_startup(fe, stream);
	if (ret < 0)
		return ret;

	ret = soc_dsp_be_dai_hw_params(fe, stream);
	if (ret < 0)
		return ret;

	ret = soc_dsp_be_dai_prepare(fe, stream);
	if (ret < 0)
		return ret;

	/* run the stream event for each BE */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->playback.stream_name,
				SNDRV_PCM_TRIGGER_START);
	else
		soc_dsp_dapm_stream_event(fe, stream,
				fe->cpu_dai->driver->capture.stream_name,
				SNDRV_PCM_TRIGGER_START);

	if (dsp_link->trigger[stream] == SND_SOC_DSP_TRIGGER_BESPOKE) {

		/* there is no point in triggering START iff all BEs are PAUSED */
		cmd = dsp_get_fe_trigger_cmd(fe, stream);

		/* call trigger on the frontend - FE takes care of all BE triggers */
		dev_dbg(&fe->dev, "dsp: bespoke trigger FE %s cmd start\n",
				fe->dai_link->name);

		ret = soc_pcm_bespoke_trigger(substream, cmd);
		if (ret < 0) {
			dev_err(&fe->dev,"dsp: trigger FE failed %d\n", ret);
			return ret;
		}

		/* successful trigger so update BE trigger status */
		list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

			/* is this op for this BE ? */
			if (fe->dsp[stream].runtime_update &&
				!dsp_params->be->dsp[stream].runtime_update)
				continue;

			switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			case SNDRV_PCM_TRIGGER_RESUME:
				dsp_params->state = SND_SOC_DSP_LINK_STATE_START;
				break;
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			case SNDRV_PCM_TRIGGER_STOP:
			case SNDRV_PCM_TRIGGER_SUSPEND:
				dsp_params->state = SND_SOC_DSP_LINK_STATE_PAUSED;
				break;
			}
		}
	} else {

		list_for_each_entry(dsp_params, &fe->dsp[stream].be_clients, list_be) {

			/* there is no point in triggering start iff all FEs are PAUSED */
			cmd = dsp_get_be_trigger_cmd(fe, stream);

			dev_dbg(&fe->dev, "dsp: trigger FE %s cmd start\n",
				fe->dai_link->name);

			ret = soc_dsp_be_dai_trigger(fe, stream, cmd);
			if (ret < 0)
				return ret;
			}
	}

	return 0;
}

static int dsp_run_update(struct snd_soc_pcm_runtime *fe, int stream,
	int start, int stop)
{
	int ret = 0;

	fe->dsp[stream].runtime_update = 1;

	/* startup any new BEs */
	if (start) {
		ret = dsp_run_update_startup(fe, stream);
		if (ret < 0)
			dev_err(&fe->dev, "failed to startup BEs\n");
	}

	/* close down old BEs */
	if (stop) {
		ret = dsp_run_update_shutdown(fe, stream);
		if (ret < 0)
			dev_err(&fe->dev, "failed to shutdown BEs\n");
	}

	fe->dsp[stream].runtime_update = 0;

	return ret;
}

/* called when any mixer updates change FE -> BE the stream */
int soc_dsp_runtime_update(struct snd_soc_dapm_widget *widget)
{
	struct snd_soc_card *card;
	int i, ret = 0, start, stop;

	if (widget->codec)
		card = widget->codec->card;
	else if (widget->platform)
		card = widget->platform->card;
	else
		return -EINVAL;

	mutex_lock(&widget->dapm->card->dsp_mutex);

	for (i = 0; i < card->num_rtd; i++) {
		struct snd_soc_pcm_runtime *fe = &card->rtd[i];

		/* make sure link is BE */
		if (!fe->dai_link->dsp_link)
			continue;

		/* only check active links */
		if (!fe->cpu_dai->active) {
			continue;
		}

		/* DAPM sync will call this to update DSP paths */
		dev_dbg(card->dev, "DSP runtime update for FE %s\n", fe->dai_link->name);

		/* update any playback paths */
		start = dsp_add_new_paths(fe, SNDRV_PCM_STREAM_PLAYBACK, 1);
		stop = dsp_prune_old_paths(fe, SNDRV_PCM_STREAM_PLAYBACK, 1);
		if (!(start || stop))
			goto capture;

		/* run PCM ops on new/old playback paths */
		ret = dsp_run_update(fe, SNDRV_PCM_STREAM_PLAYBACK, start, stop);
		if (ret < 0) {
			dev_err(&fe->dev, "failed to update playback FE stream %s\n",
					fe->dai_link->stream_name);
		}

		/* free old playback links */
		be_disconnect(fe, SNDRV_PCM_STREAM_PLAYBACK);
		fe_clear_pending(fe, SNDRV_PCM_STREAM_PLAYBACK);

capture:
		/* update any capture paths */
		start = dsp_add_new_paths(fe, SNDRV_PCM_STREAM_CAPTURE, 1);
		stop = dsp_prune_old_paths(fe, SNDRV_PCM_STREAM_CAPTURE, 1);
		if (!(start || stop))
			continue;

		/* run PCM ops on new/old capture paths */
		ret = dsp_run_update(fe, SNDRV_PCM_STREAM_CAPTURE, start, stop);
		if (ret < 0) {
			dev_err(&fe->dev, "failed to update capture FE stream %s\n",
					fe->dai_link->stream_name);
		}

		/* free old capture links */
		be_disconnect(fe, SNDRV_PCM_STREAM_CAPTURE);
		fe_clear_pending(fe, SNDRV_PCM_STREAM_CAPTURE);
	}

	mutex_unlock(&widget->dapm->card->dsp_mutex);
	return ret;
}

int soc_dsp_be_digital_mute(struct snd_soc_pcm_runtime *fe, int mute)
{
	struct snd_soc_dsp_params *dsp_params;

	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->codec_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "BE digital mute %s\n", be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->ops->digital_mute && dai->playback_active)
				drv->ops->digital_mute(dai, mute);
	}

	return 0;
}

int soc_dsp_be_cpu_dai_suspend(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* suspend for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI playback suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && !drv->ac97_control)
				drv->suspend(dai);
	}

	/* suspend for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI capture suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && !drv->ac97_control)
				drv->suspend(dai);
	}

	return 0;
}

int soc_dsp_be_ac97_cpu_dai_suspend(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* suspend for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI playback suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && drv->ac97_control)
				drv->suspend(dai);
	}

	/* suspend for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI capture suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && drv->ac97_control)
				drv->suspend(dai);
	}

	return 0;
}

int soc_dsp_be_platform_suspend(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* suspend for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_platform *platform = be->platform;
		struct snd_soc_platform_driver *drv = platform->driver;
		struct snd_soc_dai *dai = be->cpu_dai;

		dev_dbg(&be->dev, "pm: BE platform playback suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && !platform->suspended) {
			drv->suspend(dai);
			platform->suspended = 1;
		}
	}

	/* suspend for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_platform *platform = be->platform;
		struct snd_soc_platform_driver *drv = platform->driver;
		struct snd_soc_dai *dai = be->cpu_dai;

		dev_dbg(&be->dev, "pm: BE platform capture suspend %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->suspend && !platform->suspended) {
			drv->suspend(dai);
			platform->suspended = 1;
		}
	}

	return 0;
}

int soc_dsp_be_cpu_dai_resume(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* resume for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI playback resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && !drv->ac97_control)
				drv->resume(dai);
	}

	/* suspend for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI capture resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && !drv->ac97_control)
				drv->resume(dai);
	}

	return 0;
}

int soc_dsp_be_ac97_cpu_dai_resume(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* resume for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI playback resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && drv->ac97_control)
				drv->resume(dai);
	}

	/* suspend for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_dai *dai = be->cpu_dai;
		struct snd_soc_dai_driver *drv = dai->driver;

		dev_dbg(&be->dev, "pm: BE CPU DAI capture resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && drv->ac97_control)
				drv->resume(dai);
	}

	return 0;
}

int soc_dsp_be_platform_resume(struct snd_soc_pcm_runtime *fe)
{
	struct snd_soc_dsp_params *dsp_params;

	/* resume for playback */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_PLAYBACK].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_platform *platform = be->platform;
		struct snd_soc_platform_driver *drv = platform->driver;
		struct snd_soc_dai *dai = be->cpu_dai;

		dev_dbg(&be->dev, "pm: BE platform playback resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && platform->suspended) {
			drv->resume(dai);
			platform->suspended = 0;
		}
	}

	/* resume for capture */
	list_for_each_entry(dsp_params,
			&fe->dsp[SNDRV_PCM_STREAM_CAPTURE].be_clients, list_be) {

		struct snd_soc_pcm_runtime *be = dsp_params->be;
		struct snd_soc_platform *platform = be->platform;
		struct snd_soc_platform_driver *drv = platform->driver;
		struct snd_soc_dai *dai = be->cpu_dai;

		dev_dbg(&be->dev, "pm: BE platform capture resume %s\n",
				be->dai_link->name);

		if (be->dai_link->ignore_suspend)
			continue;

		if (drv->resume && platform->suspended) {
			drv->resume(dai);
			platform->suspended = 0;
		}
	}

	return 0;
}

/* called when opening FE stream  */
int soc_dsp_fe_dai_open(struct snd_pcm_substream *fe_substream)
{
	struct snd_soc_pcm_runtime *fe = fe_substream->private_data;
	int err;

	fe->dsp[fe_substream->stream].runtime = fe_substream->runtime;

	/* calculate valid and active FE <-> BE dsp_paramss */
	err = dsp_add_new_paths(fe, fe_substream->stream, 0);
	if (err <= 0) {
		dev_warn(&fe->dev, "asoc: %s no valid %s route from source to sink\n",
			fe->dai_link->name, fe_substream->stream ? "capture" : "playback");
	}

	return soc_dsp_fe_dai_startup(fe_substream);
}

/* called when closing FE stream  */
int soc_dsp_fe_dai_close(struct snd_pcm_substream *fe_substream)
{
	struct snd_soc_pcm_runtime *fe = fe_substream->private_data;
	int ret;

	ret = soc_dsp_fe_dai_shutdown(fe_substream);

	be_disconnect(fe, fe_substream->stream);

	return ret;
}

#ifdef CONFIG_DEBUG_FS
int soc_dsp_debugfs_add(struct snd_soc_pcm_runtime *rtd)
{
	rtd->debugfs_dsp_root = debugfs_create_dir(rtd->dai_link->name,
			rtd->card->debugfs_card_root);
	if (!rtd->debugfs_dsp_root) {
		dev_dbg(&rtd->dev,
			 "ASoC: Failed to create dsp debugfs directory %s\n",
			 rtd->dai_link->name);
		return -EINVAL;
	}

	return 0;
}
#endif

/* create a new pcm */
int soc_new_pcm(struct snd_soc_pcm_runtime *rtd, int num)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_platform *platform = rtd->platform;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_pcm_substream *substream[2];
	struct snd_pcm *pcm;
	char new_name[64];
	int ret = 0, playback = 0, capture = 0;

	/* check client and interface hw capabilities */
	snprintf(new_name, sizeof(new_name), "%s %s-%d",
			rtd->dai_link->stream_name, codec_dai->name, num);

	if (rtd->dai_link->dynamic) {
		playback = rtd->dai_link->dsp_link->playback;
		capture = rtd->dai_link->dsp_link->capture;
	} else {
		if (codec_dai->driver->playback.channels_min)
			playback = 1;
		if (codec_dai->driver->capture.channels_min)
			capture = 1;
	}

	dev_dbg(rtd->card->dev, "registered pcm #%d %s\n",num,new_name);
	ret = snd_pcm_new(rtd->card->snd_card, new_name,
			num, playback, capture, &pcm);
	if (ret < 0) {
		printk(KERN_ERR "asoc: can't create pcm for codec %s\n", codec->name);
		return ret;
	}

	/* DAPM dai link stream work */
	INIT_DELAYED_WORK(&rtd->delayed_work, close_delayed_work);

	rtd->pcm = pcm;
	pcm->private_data = rtd;

	substream[SNDRV_PCM_STREAM_PLAYBACK] =
			pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
	substream[SNDRV_PCM_STREAM_CAPTURE] =
			pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;

	if (rtd->dai_link->no_pcm) {
		if (playback)
			substream[SNDRV_PCM_STREAM_PLAYBACK]->private_data = rtd;
		if (capture)
			substream[SNDRV_PCM_STREAM_CAPTURE]->private_data = rtd;
		goto out;
	}

	/* setup any hostless PCMs - i.e. no host IO is performed */
	if (rtd->dai_link->no_host_mode) {
		substream[SNDRV_PCM_STREAM_PLAYBACK]->hw_no_buffer = 1;
		substream[SNDRV_PCM_STREAM_CAPTURE]->hw_no_buffer = 1;
		snd_soc_set_runtime_hwparams(substream[SNDRV_PCM_STREAM_PLAYBACK],
				&no_host_hardware);
		snd_soc_set_runtime_hwparams(substream[SNDRV_PCM_STREAM_CAPTURE],
				&no_host_hardware);
	}


	/* ASoC PCM operations */
	if (rtd->dai_link->dynamic) {
		rtd->ops.open		= soc_dsp_fe_dai_open;
		rtd->ops.hw_params	= soc_dsp_fe_dai_hw_params;
		rtd->ops.prepare	= soc_dsp_fe_dai_prepare;
		rtd->ops.trigger	= soc_dsp_fe_dai_trigger;
		rtd->ops.hw_free	= soc_dsp_fe_dai_hw_free;
		rtd->ops.close		= soc_dsp_fe_dai_close;
	} else {
		rtd->ops.open		= soc_pcm_open;
		rtd->ops.hw_params	= soc_pcm_hw_params;
		rtd->ops.prepare	= soc_pcm_prepare;
		rtd->ops.trigger	= soc_pcm_trigger;
		rtd->ops.hw_free	= soc_pcm_hw_free;
		rtd->ops.close		= soc_pcm_close;
	}
	rtd->ops.pointer	= soc_pcm_pointer;
	rtd->ops.ioctl		= soc_pcm_ioctl;

	if (platform->driver->ops) {
		rtd->ops.ack		= platform->driver->ops->ack;
		rtd->ops.copy		= platform->driver->ops->copy;
		rtd->ops.silence	= platform->driver->ops->silence;
		rtd->ops.page		= platform->driver->ops->page;
		rtd->ops.mmap		= platform->driver->ops->mmap;
	}

	if (playback)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &rtd->ops);

	if (capture)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &rtd->ops);

	if (platform->driver->pcm_new) {
		ret = platform->driver->pcm_new(rtd);
		if (ret < 0) {
			pr_err("asoc: platform pcm constructor failed\n");
			return ret;
		}
	}

	pcm->private_free = platform->driver->pcm_free;

out:
	printk(KERN_INFO "asoc: %s <-> %s mapping ok\n", codec_dai->name,
		cpu_dai->name);
	return ret;
}
