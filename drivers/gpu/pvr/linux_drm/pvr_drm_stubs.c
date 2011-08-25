/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <asm/system.h>

#include "pvr_drm_mod.h"

#define	DRV_MSG_PREFIX_STR "pvr drm: "

#define	SGX_VENDOR_ID		1
#define	SGX_DEVICE_ID		1
#define	SGX_SUB_VENDOR_ID	1
#define	SGX_SUB_DEVICE_ID	1

#if defined(DEBUG)
#define	DEBUG_PRINTK(format, args...) printk(format, ## args)
#else
#define	DEBUG_PRINTK(format, args...)
#endif

#define	CLEAR_STRUCT(x) memset(&(x), 0, sizeof(x))

static struct pci_bus pvr_pci_bus;
static struct pci_dev pvr_pci_dev;

static bool bDeviceIsRegistered;

static void
release_device(struct device *dev)
{
}

int
drm_pvr_dev_add(void)
{
	int ret;

	DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s\n", __FUNCTION__);

	if (bDeviceIsRegistered)
	{
		DEBUG_PRINTK(KERN_WARNING DRV_MSG_PREFIX_STR "%s: Device already registered\n", __FUNCTION__);
		return 0;
	}

	
	pvr_pci_dev.vendor = SGX_VENDOR_ID;
	pvr_pci_dev.device = SGX_DEVICE_ID;
	pvr_pci_dev.subsystem_vendor = SGX_SUB_VENDOR_ID;
	pvr_pci_dev.subsystem_device = SGX_SUB_DEVICE_ID;

	
	pvr_pci_dev.bus = &pvr_pci_bus;

	dev_set_name(&pvr_pci_dev.dev, "%s", "SGX");
	pvr_pci_dev.dev.release = release_device;

	ret = device_register(&pvr_pci_dev.dev);
	if (ret != 0)
	{
		printk(KERN_ERR DRV_MSG_PREFIX_STR "%s: device_register failed (%d)\n", __FUNCTION__, ret);
	}

	bDeviceIsRegistered = true;

	return ret;
}
EXPORT_SYMBOL(drm_pvr_dev_add);

void
drm_pvr_dev_remove(void)
{
	DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s\n", __FUNCTION__);

	if (bDeviceIsRegistered)
	{
		DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Unregistering device\n", __FUNCTION__);

		device_unregister(&pvr_pci_dev.dev);
		bDeviceIsRegistered = false;

		
		CLEAR_STRUCT(pvr_pci_dev);
		CLEAR_STRUCT(pvr_pci_bus);
	}
	else
	{
		DEBUG_PRINTK(KERN_WARNING DRV_MSG_PREFIX_STR "%s: Device not registered\n", __FUNCTION__);
	}
}
EXPORT_SYMBOL(drm_pvr_dev_remove);

void
pci_disable_device(struct pci_dev *dev)
{
}

struct pci_dev *
pci_dev_get(struct pci_dev *dev)
{
	return dev;
}

void
pci_set_master(struct pci_dev *dev)
{
}

#define	PCI_ID_COMP(field, value) (((value) == PCI_ANY_ID) || \
			((field) == (value)))

struct pci_dev *
pci_get_subsys(unsigned int vendor, unsigned int device,
	unsigned int ss_vendor, unsigned int ss_device, struct pci_dev *from)
{
	if (from == NULL &&
		PCI_ID_COMP(pvr_pci_dev.vendor, vendor) &&
		PCI_ID_COMP(pvr_pci_dev.device, device) &&
		PCI_ID_COMP(pvr_pci_dev.subsystem_vendor, ss_vendor) &&
		PCI_ID_COMP(pvr_pci_dev.subsystem_device, ss_device))
	{
			DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Found %x %x %x %x\n", __FUNCTION__, vendor, device, ss_vendor, ss_device);

			return &pvr_pci_dev;
	}

	if (from == NULL)
	{
		DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Couldn't find %x %x %x %x\n", __FUNCTION__, vendor, device, ss_vendor, ss_device);
	}

	return NULL;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
int
pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
	return 0;
}
#endif

void
pci_unregister_driver(struct pci_driver *drv)
{
}

int
__pci_register_driver(struct pci_driver *drv, struct module *owner,
	const char *mod_name)
{
	return 0;
}

int
pci_enable_device(struct pci_dev *dev)
{
	return 0;
}

void
__bad_cmpxchg(volatile void *ptr, int size)
{
	printk(KERN_ERR DRV_MSG_PREFIX_STR "%s: ptr %p size %u\n",
		__FUNCTION__, ptr, size);
}

