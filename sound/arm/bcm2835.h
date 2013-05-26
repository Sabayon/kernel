/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*	
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

#ifndef __SOUND_ARM_BCM2835_H
#define __SOUND_ARM_BCM2835_H

#include <linux/device.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/workqueue.h>

/*
#define AUDIO_DEBUG_ENABLE
#define AUDIO_VERBOSE_DEBUG_ENABLE
*/

/* Debug macros */

#ifdef AUDIO_DEBUG_ENABLE
#ifdef AUDIO_VERBOSE_DEBUG_ENABLE

#define audio_debug(fmt, arg...)	\
	printk(KERN_INFO"%s:%d " fmt, __func__, __LINE__, ##arg)

#define audio_info(fmt, arg...)	\
	printk(KERN_INFO"%s:%d " fmt, __func__, __LINE__, ##arg)

#else

#define audio_debug(fmt, arg...)

#define audio_info(fmt, arg...)

#endif /* AUDIO_VERBOSE_DEBUG_ENABLE */

#else

#define audio_debug(fmt, arg...)

#define audio_info(fmt, arg...)

#endif /* AUDIO_DEBUG_ENABLE */

#define audio_error(fmt, arg...)	\
	printk(KERN_ERR"%s:%d " fmt, __func__, __LINE__, ##arg)

#define audio_warning(fmt, arg...)	\
	printk(KERN_WARNING"%s:%d " fmt, __func__, __LINE__, ##arg)

#define audio_alert(fmt, arg...)	\
	printk(KERN_ALERT"%s:%d " fmt, __func__, __LINE__, ##arg)

#define MAX_SUBSTREAMS			(8)
#define AVAIL_SUBSTREAMS_MASK		(0xff)
enum {
	CTRL_VOL_MUTE,
	CTRL_VOL_UNMUTE
};

/* macros for alsa2chip and chip2alsa, instead of functions */

#define alsa2chip(vol) (uint)(-((vol << 8) / 100))	/* convert alsa to chip volume (defined as macro rather than function call) */
#define chip2alsa(vol) -((vol * 100) >> 8)			/* convert chip to alsa volume */

/* Some constants for values .. */
typedef enum {
	AUDIO_DEST_AUTO = 0,
	AUDIO_DEST_HEADPHONES = 1,
	AUDIO_DEST_HDMI = 2,
	AUDIO_DEST_MAX,
} SND_BCM2835_ROUTE_T;

typedef enum {
	PCM_PLAYBACK_VOLUME,
	PCM_PLAYBACK_MUTE,
	PCM_PLAYBACK_DEVICE,
} SND_BCM2835_CTRL_T;

/* definition of the chip-specific record */
typedef struct bcm2835_chip {
	struct snd_card *card;
	struct snd_pcm *pcm;
	/* Bitmat for valid reg_base and irq numbers */
	uint32_t avail_substreams;
	struct platform_device *pdev[MAX_SUBSTREAMS];
	struct bcm2835_alsa_stream *alsa_stream[MAX_SUBSTREAMS];

	int volume;
	int old_volume; /* stores the volume value whist muted */
	int dest;
	int mute;
} bcm2835_chip_t;

typedef struct bcm2835_alsa_stream {
	bcm2835_chip_t *chip;
	struct snd_pcm_substream *substream;

	struct semaphore buffers_update_sem;
	struct semaphore control_sem;
	spinlock_t lock;
	volatile uint32_t control;
	volatile uint32_t status;

	int open;
	int running;
	int draining;

	unsigned int pos;
	unsigned int buffer_size;
	unsigned int period_size;

	uint32_t enable_fifo_irq;
	irq_handler_t fifo_irq_handler;

	atomic_t retrieved;
	struct opaque_AUDIO_INSTANCE_T *instance;
	struct workqueue_struct *my_wq;
	int idx;
} bcm2835_alsa_stream_t;

int snd_bcm2835_new_ctl(bcm2835_chip_t * chip);
int snd_bcm2835_new_pcm(bcm2835_chip_t * chip);

int bcm2835_audio_open(bcm2835_alsa_stream_t * alsa_stream);
int bcm2835_audio_close(bcm2835_alsa_stream_t * alsa_stream);
int bcm2835_audio_set_params(bcm2835_alsa_stream_t * alsa_stream,
			     uint32_t channels, uint32_t samplerate,
			     uint32_t bps);
int bcm2835_audio_setup(bcm2835_alsa_stream_t * alsa_stream);
int bcm2835_audio_start(bcm2835_alsa_stream_t * alsa_stream);
int bcm2835_audio_stop(bcm2835_alsa_stream_t * alsa_stream);
int bcm2835_audio_set_ctls(bcm2835_chip_t * chip);
int bcm2835_audio_write(bcm2835_alsa_stream_t * alsa_stream, uint32_t count,
			void *src);
uint32_t bcm2835_audio_retrieve_buffers(bcm2835_alsa_stream_t * alsa_stream);
void bcm2835_audio_flush_buffers(bcm2835_alsa_stream_t * alsa_stream);
void bcm2835_audio_flush_playback_buffers(bcm2835_alsa_stream_t * alsa_stream);

#endif /* __SOUND_ARM_BCM2835_H */
