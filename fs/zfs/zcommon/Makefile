MODULE := zcommon

EXTRA_CFLAGS = $(ZFS_MODULE_CFLAGS)  -Wno-unused-but-set-variable -DHAVE_SPL -D_KERNEL -DTEXT_DOMAIN=\"zfs-linux-kernel\" -DNDEBUG 

obj-$(CONFIG_ZFS) := $(MODULE).o

$(MODULE)-objs += zfs_deleg.o
$(MODULE)-objs += zfs_prop.o
$(MODULE)-objs += zprop_common.o
$(MODULE)-objs += zfs_namecheck.o
$(MODULE)-objs += zfs_comutil.o
$(MODULE)-objs += zfs_fletcher.o
$(MODULE)-objs += zfs_uio.o
$(MODULE)-objs += zpool_prop.o
