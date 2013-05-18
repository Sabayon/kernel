#
# Copyright (C) 2010-2011 ARM Limited. All rights reserved.
# 
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
# 
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

ifeq ($(CONFIG_UMP_USING_OS_MEMORY),y)
USING_MEMORY_TYPE=1
endif

# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH

## @note Should allow overriding of building UMP for non-debug:

ifeq ($(CONFIG_UMP_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
EXTRA_CFLAGS += -DMALI_STATE_TRACKING=0
USING_MEMORY_TYPE ?=0 # 0: dedicated memory 1: OS memory

EXTRA_CFLAGS += -I$(src) -I$(src)/common -I$(src)/linux -I$(src)/include -I$(src)/linux/license/gpl
EXTRA_CFLAGS += -I$(src)/../mali/common -I$(src)/../mali/linux

EXTRA_CFLAGS += -DUMP_MEMORY_TYPE=$(USING_MEMORY_TYPE)

obj-$(CONFIG_UMP) += ump.o

MALI_OSKFILES =
ifneq ($(CONFIG_MALI400),y)
MALI_OSKFILES = ../mali/linux/mali_osk_atomics.o \
            ../mali/linux/mali_osk_locks.o \
            ../mali/linux/mali_osk_math.o \
            ../mali/linux/mali_osk_memory.o \
            ../mali/linux/mali_osk_misc.o
endif

ump-y:=	common/ump_kernel_common.o \
	common/ump_kernel_descriptor_mapping.o \
	common/ump_kernel_api.o \
	common/ump_kernel_ref_drv.o \
	linux/ump_kernel_linux.o \
	linux/ump_kernel_memory_backend_os.o \
	linux/ump_kernel_memory_backend_dedicated.o \
	linux/ump_memory_backend.o \
	linux/ump_ukk_wrappers.o \
	linux/ump_ukk_ref_wrappers.o \
	linux/ump_osk_atomics.o \
	linux/ump_osk_low_level_mem.o \
	linux/ump_osk_misc.o \
	$(MALI_OSKFILES)


SVN_REV:=$(shell cat $(src)/.version 2> /dev/null)

EXTRA_CFLAGS += -DSVN_REV=$(SVN_REV)
EXTRA_CFLAGS += -DSVN_REV_STRING=\"$(SVN_REV)\"

