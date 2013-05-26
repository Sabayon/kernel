/*
 *  linux/drivers/video/bcm2708_fb.c
 *
 * Copyright (C) 2010 Broadcom
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Broadcom simple framebuffer driver
 *
 * This file is derived from cirrusfb.c
 * Copyright 1999-2001 Jeff Garzik <jgarzik@pobox.com>
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/printk.h>
#include <linux/console.h>

#include <mach/platform.h>
#include <mach/vcio.h>

#include <asm/sizes.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>

#ifdef BCM2708_FB_DEBUG
#define print_debug(fmt,...) pr_debug("%s:%s:%d: "fmt, MODULE_NAME, __func__, __LINE__, ##__VA_ARGS__)
#else
#define print_debug(fmt,...)
#endif

/* This is limited to 16 characters when displayed by X startup */
static const char *bcm2708_name = "BCM2708 FB";

#define DRIVER_NAME "bcm2708_fb"

/* this data structure describes each frame buffer device we find */

struct fbinfo_s {
	u32 xres, yres, xres_virtual, yres_virtual;
	u32 pitch, bpp;
	u32 xoffset, yoffset;
	u32 base;
	u32 screen_size;
	u16 cmap[256];
};

struct bcm2708_fb {
	struct fb_info fb;
	struct platform_device *dev;
	struct fbinfo_s *info;
	dma_addr_t dma;
	u32 cmap[16];
};

#define to_bcm2708(info)	container_of(info, struct bcm2708_fb, fb)

static int bcm2708_fb_set_bitfields(struct fb_var_screeninfo *var)
{
	int ret = 0;

	memset(&var->transp, 0, sizeof(var->transp));

	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;

	switch (var->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
		var->red.length = var->bits_per_pixel;
		var->red.offset = 0;
		var->green.length = var->bits_per_pixel;
		var->green.offset = 0;
		var->blue.length = var->bits_per_pixel;
		var->blue.offset = 0;
		break;
	case 16:
		var->red.length = 5;
		var->blue.length = 5;
		/*
		 * Green length can be 5 or 6 depending whether
		 * we're operating in RGB555 or RGB565 mode.
		 */
		if (var->green.length != 5 && var->green.length != 6)
			var->green.length = 6;
		break;
	case 24:
		var->red.length = 8;
		var->blue.length = 8;
		var->green.length = 8;
		break;
	case 32:
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 8;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	/*
	 * >= 16bpp displays have separate colour component bitfields
	 * encoded in the pixel data.  Calculate their position from
	 * the bitfield length defined above.
	 */
	if (ret == 0 && var->bits_per_pixel >= 24) {
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;
		var->transp.offset = var->blue.offset + var->blue.length;
	} else if (ret == 0 && var->bits_per_pixel >= 16) {
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		var->transp.offset = var->red.offset + var->red.length;
	}

	return ret;
}

static int bcm2708_fb_check_var(struct fb_var_screeninfo *var,
				struct fb_info *info)
{
	/* info input, var output */
	int yres;

	/* info input, var output */
	print_debug("bcm2708_fb_check_var info(%p) %dx%d (%dx%d), %d, %d\n", info,
		info->var.xres, info->var.yres, info->var.xres_virtual,
		info->var.yres_virtual, (int)info->screen_size,
		info->var.bits_per_pixel);
	print_debug("bcm2708_fb_check_var var(%p) %dx%d (%dx%d), %d\n", var,
		var->xres, var->yres, var->xres_virtual, var->yres_virtual,
		var->bits_per_pixel);

	if (!var->bits_per_pixel)
		var->bits_per_pixel = 16;

	if (bcm2708_fb_set_bitfields(var) != 0) {
		pr_err("bcm2708_fb_check_var: invalid bits_per_pixel %d\n",
		     var->bits_per_pixel);
		return -EINVAL;
	}


	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	/* use highest possible virtual resolution */
	if (var->yres_virtual == -1) {
		var->yres_virtual = 480;

		pr_err
		    ("bcm2708_fb_check_var: virtual resolution set to maximum of %dx%d\n",
		     var->xres_virtual, var->yres_virtual);
	}
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	if (var->xoffset < 0)
		var->xoffset = 0;
	if (var->yoffset < 0)
		var->yoffset = 0;

	/* truncate xoffset and yoffset to maximum if too high */
	if (var->xoffset > var->xres_virtual - var->xres)
		var->xoffset = var->xres_virtual - var->xres - 1;
	if (var->yoffset > var->yres_virtual - var->yres)
		var->yoffset = var->yres_virtual - var->yres - 1;

	yres = var->yres;
	if (var->vmode & FB_VMODE_DOUBLE)
		yres *= 2;
	else if (var->vmode & FB_VMODE_INTERLACED)
		yres = (yres + 1) / 2;

	if (yres > 1200) {
		pr_err("bcm2708_fb_check_var: ERROR: VerticalTotal >= 1200; "
		       "special treatment required! (TODO)\n");
		return -EINVAL;
	}

	return 0;
}

static int bcm2708_fb_set_par(struct fb_info *info)
{
	uint32_t val = 0;
	struct bcm2708_fb *fb = to_bcm2708(info);
	volatile struct fbinfo_s *fbinfo = fb->info;
	fbinfo->xres = info->var.xres;
	fbinfo->yres = info->var.yres;
	fbinfo->xres_virtual = info->var.xres_virtual;
	fbinfo->yres_virtual = info->var.yres_virtual;
	fbinfo->bpp = info->var.bits_per_pixel;
	fbinfo->xoffset = info->var.xoffset;
	fbinfo->yoffset = info->var.yoffset;
	fbinfo->base = 0;	/* filled in by VC */
	fbinfo->pitch = 0;	/* filled in by VC */

	print_debug("bcm2708_fb_set_par info(%p) %dx%d (%dx%d), %d, %d\n", info,
		info->var.xres, info->var.yres, info->var.xres_virtual,
		info->var.yres_virtual, (int)info->screen_size,
		info->var.bits_per_pixel);

	/* ensure last write to fbinfo is visible to GPU */
	wmb();

	/* inform vc about new framebuffer */
	bcm_mailbox_write(MBOX_CHAN_FB, fb->dma);

	/* TODO: replace fb driver with vchiq version */
	/* wait for response */
	bcm_mailbox_read(MBOX_CHAN_FB, &val);

	/* ensure GPU writes are visible to us */
	rmb();

        if (val == 0) {
		fb->fb.fix.line_length = fbinfo->pitch;

		if (info->var.bits_per_pixel <= 8)
			fb->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;
		else
			fb->fb.fix.visual = FB_VISUAL_TRUECOLOR;

		fb->fb.fix.smem_start = fbinfo->base;
		fb->fb.fix.smem_len = fbinfo->pitch * fbinfo->yres_virtual;
		fb->fb.screen_size = fbinfo->screen_size;
		if (fb->fb.screen_base)
			iounmap(fb->fb.screen_base);
		fb->fb.screen_base =
			(void *)ioremap_wc(fb->fb.fix.smem_start, fb->fb.screen_size);
		if (!fb->fb.screen_base) {
			/* the console may currently be locked */
			console_trylock();
			console_unlock();

			BUG();		/* what can we do here */
		}
	}
	print_debug
	    ("BCM2708FB: start = %p,%p width=%d, height=%d, bpp=%d, pitch=%d size=%d success=%d\n",
	     (void *)fb->fb.screen_base, (void *)fb->fb.fix.smem_start,
	     fbinfo->xres, fbinfo->yres, fbinfo->bpp,
	     fbinfo->pitch, (int)fb->fb.screen_size, val);

	return val;
}

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	unsigned int mask = (1 << bf->length) - 1;

	return (val >> (16 - bf->length) & mask) << bf->offset;
}


static int bcm2708_fb_setcolreg(unsigned int regno, unsigned int red,
				unsigned int green, unsigned int blue,
				unsigned int transp, struct fb_info *info)
{
	struct bcm2708_fb *fb = to_bcm2708(info);

	/*print_debug("BCM2708FB: setcolreg %d:(%02x,%02x,%02x,%02x) %x\n", regno, red, green, blue, transp, fb->fb.fix.visual);*/
	if (fb->fb.var.bits_per_pixel <= 8) {
		if (regno < 256) {
			/* blue [0:4], green [5:10], red [11:15] */
			fb->info->cmap[regno] = ((red   >> (16-5)) & 0x1f) << 11 |
						((green >> (16-6)) & 0x3f) << 5 |
						((blue  >> (16-5)) & 0x1f) << 0;
		}
		/* Hack: we need to tell GPU the palette has changed, but currently bcm2708_fb_set_par takes noticable time when called for every (256) colour */
		/* So just call it for what looks like the last colour in a list for now. */
		if (regno == 15 || regno == 255)
			bcm2708_fb_set_par(info);
        } else if (regno < 16) {
		fb->cmap[regno] = convert_bitfield(transp, &fb->fb.var.transp) |
		    convert_bitfield(blue, &fb->fb.var.blue) |
		    convert_bitfield(green, &fb->fb.var.green) |
		    convert_bitfield(red, &fb->fb.var.red);
	}
	return regno > 255;
}

static int bcm2708_fb_blank(int blank_mode, struct fb_info *info)
{
	/*print_debug("bcm2708_fb_blank\n"); */
	return -1;
}

static void bcm2708_fb_fillrect(struct fb_info *info,
				const struct fb_fillrect *rect)
{
	/* (is called) print_debug("bcm2708_fb_fillrect\n"); */
	cfb_fillrect(info, rect);
}

static void bcm2708_fb_copyarea(struct fb_info *info,
				const struct fb_copyarea *region)
{
	/*print_debug("bcm2708_fb_copyarea\n"); */
	cfb_copyarea(info, region);
}

static void bcm2708_fb_imageblit(struct fb_info *info,
				 const struct fb_image *image)
{
	/* (is called) print_debug("bcm2708_fb_imageblit\n"); */
	cfb_imageblit(info, image);
}

static struct fb_ops bcm2708_fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = bcm2708_fb_check_var,
	.fb_set_par = bcm2708_fb_set_par,
	.fb_setcolreg = bcm2708_fb_setcolreg,
	.fb_blank = bcm2708_fb_blank,
	.fb_fillrect = bcm2708_fb_fillrect,
	.fb_copyarea = bcm2708_fb_copyarea,
	.fb_imageblit = bcm2708_fb_imageblit,
};

static int fbwidth = 800;	/* module parameter */
static int fbheight = 480;	/* module parameter */
static int fbdepth = 16;	/* module parameter */

static int bcm2708_fb_register(struct bcm2708_fb *fb)
{
	int ret;
	dma_addr_t dma;
	void *mem;

	mem =
	    dma_alloc_coherent(NULL, PAGE_ALIGN(sizeof(*fb->info)), &dma,
			       GFP_KERNEL);

	if (NULL == mem) {
		pr_err(": unable to allocate fbinfo buffer\n");
		ret = -ENOMEM;
	} else {
		fb->info = (struct fbinfo_s *)mem;
		fb->dma = dma;
	}
	fb->fb.fbops = &bcm2708_fb_ops;
	fb->fb.flags = FBINFO_FLAG_DEFAULT;
	fb->fb.pseudo_palette = fb->cmap;

	strncpy(fb->fb.fix.id, bcm2708_name, sizeof(fb->fb.fix.id));
	fb->fb.fix.type = FB_TYPE_PACKED_PIXELS;
	fb->fb.fix.type_aux = 0;
	fb->fb.fix.xpanstep = 0;
	fb->fb.fix.ypanstep = 0;
	fb->fb.fix.ywrapstep = 0;
	fb->fb.fix.accel = FB_ACCEL_NONE;

	fb->fb.var.xres = fbwidth;
	fb->fb.var.yres = fbheight;
	fb->fb.var.xres_virtual = fbwidth;
	fb->fb.var.yres_virtual = fbheight;
	fb->fb.var.bits_per_pixel = fbdepth;
	fb->fb.var.vmode = FB_VMODE_NONINTERLACED;
	fb->fb.var.activate = FB_ACTIVATE_NOW;
	fb->fb.var.nonstd = 0;
	fb->fb.var.height = -1;		/* height of picture in mm    */
	fb->fb.var.width = -1;		/* width of picture in mm    */
	fb->fb.var.accel_flags = 0;

	fb->fb.monspecs.hfmin = 0;
	fb->fb.monspecs.hfmax = 100000;
	fb->fb.monspecs.vfmin = 0;
	fb->fb.monspecs.vfmax = 400;
	fb->fb.monspecs.dclkmin = 1000000;
	fb->fb.monspecs.dclkmax = 100000000;

	bcm2708_fb_set_bitfields(&fb->fb.var);

	/*
	 * Allocate colourmap.
	 */

	fb_set_var(&fb->fb, &fb->fb.var);

	print_debug("BCM2708FB: registering framebuffer (%dx%d@%d)\n", fbwidth,
		fbheight, fbdepth);

	ret = register_framebuffer(&fb->fb);
	print_debug("BCM2708FB: register framebuffer (%d)\n", ret);
	if (ret == 0)
		goto out;

	print_debug("BCM2708FB: cannot register framebuffer (%d)\n", ret);
out:
	return ret;
}

static int bcm2708_fb_probe(struct platform_device *dev)
{
	struct bcm2708_fb *fb;
	int ret;

	fb = kmalloc(sizeof(struct bcm2708_fb), GFP_KERNEL);
	if (!fb) {
		dev_err(&dev->dev,
			"could not allocate new bcm2708_fb struct\n");
		ret = -ENOMEM;
		goto free_region;
	}
	memset(fb, 0, sizeof(struct bcm2708_fb));

	fb->dev = dev;

	ret = bcm2708_fb_register(fb);
	if (ret == 0) {
		platform_set_drvdata(dev, fb);
		goto out;
	}

	kfree(fb);
free_region:
	dev_err(&dev->dev, "probe failed, err %d\n", ret);
out:
	return ret;
}

static int bcm2708_fb_remove(struct platform_device *dev)
{
	struct bcm2708_fb *fb = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	if (fb->fb.screen_base)
		iounmap(fb->fb.screen_base);
	unregister_framebuffer(&fb->fb);

	dma_free_coherent(NULL, PAGE_ALIGN(sizeof(*fb->info)), (void *)fb->info,
			  fb->dma);
	kfree(fb);

	return 0;
}

static struct platform_driver bcm2708_fb_driver = {
	.probe = bcm2708_fb_probe,
	.remove = bcm2708_fb_remove,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init bcm2708_fb_init(void)
{
	return platform_driver_register(&bcm2708_fb_driver);
}

module_init(bcm2708_fb_init);

static void __exit bcm2708_fb_exit(void)
{
	platform_driver_unregister(&bcm2708_fb_driver);
}

module_exit(bcm2708_fb_exit);

module_param(fbwidth, int, 0644);
module_param(fbheight, int, 0644);
module_param(fbdepth, int, 0644);

MODULE_DESCRIPTION("BCM2708 framebuffer driver");
MODULE_LICENSE("GPL");

MODULE_PARM_DESC(fbwidth, "Width of ARM Framebuffer");
MODULE_PARM_DESC(fbheight, "Height of ARM Framebuffer");
MODULE_PARM_DESC(fbdepth, "Bit depth of ARM Framebuffer");
