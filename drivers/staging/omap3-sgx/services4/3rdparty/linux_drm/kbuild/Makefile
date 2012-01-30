#
# Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
# 
# This program is distributed in the hope it will be useful but, except 
# as otherwise stated in writing, without any warranty; without even the 
# implied warranty of merchantability or fitness for a particular purpose. 
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
# 
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#
# Contact Information:
# Imagination Technologies Ltd. <gpl-support@imgtec.com>
# Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
# 
#

MODULE		= drm

INCLUDES =

SOURCES	=

ifneq ($(SUPPORT_DRI_DRM),1)
$(error "SUPPORT_DRI_DRM must be set")
endif

SOURCES_ROOT = ..

INT_SOURCE_LIST += \
	pvr_drm_stubs.c

INT_SOURCES := $(addprefix $(SOURCES_ROOT)/,$(INT_SOURCE_LIST))
SOURCES += $(INT_SOURCES)

EXT_SOURCE_DIR := $(KERNELDIR)/drivers/gpu/drm

EXT_BUILD_DIR := $(SOURCES_ROOT)/kbuild/tmp_$(PVR_BUILD_DIR)_$(BUILD)_$(MODULE)

$(src)/$(EXT_BUILD_DIR)/%.c: $(EXT_SOURCE_DIR)/%.c
	$(SILENT)$(MKDIR) $(dir $@)
	$(SILENT)$(CP) $< $@

clean-dirs += $(EXT_BUILD_DIR)

EXT_SOURCE_LIST := \
	drm_auth.c drm_bufs.c drm_cache.c \
	drm_context.c drm_dma.c drm_drawable.c \
	drm_drv.c drm_fops.c drm_gem.c drm_ioctl.c drm_irq.c \
	drm_lock.c drm_memory.c drm_proc.c drm_stub.c drm_vm.c \
	drm_agpsupport.c drm_scatter.c ati_pcigart.c drm_pci.c \
	drm_sysfs.c drm_hashtab.c drm_sman.c drm_mm.c \
	drm_crtc.c drm_modes.c drm_edid.c \
	drm_info.c drm_debugfs.c drm_encoder_slave.c

EXT_SOURCES := $(addprefix $(EXT_BUILD_DIR)/,$(EXT_SOURCE_LIST))
SOURCES += $(EXT_SOURCES)

INCLUDES += \
		-I$(KERNELDIR)/include/drm \
		-I$(EXT_SOURCE_DIR)

MODULE_CFLAGS += -DCONFIG_PCI -Wno-error

include $(EURASIAROOT)/eurasiacon/build/linux/kbuild/Makefile.kbuild_subdir_common
