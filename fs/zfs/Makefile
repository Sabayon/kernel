subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

ZFS_MODULE_CFLAGS += -include /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/spl_config.h
ZFS_MODULE_CFLAGS += -include /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/zfs-src/zfs_config.h
ZFS_MODULE_CFLAGS += -I/var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/zfs-src/include -I/var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/include -I/var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/Module.symvers ]; then \
		/bin/cp /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/Module.symvers .; \
	elif [ -f /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/module/Module.symvers ]; then \
		/bin/cp /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/module/Module.symvers .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/Module.symvers, or\n" \
		"*** - /var/tmp/portage/sys-fs/zfs-20120903/work/zfs-20120903/spl-src/module/Module.symvers\n"; \
		exit 1; \
	fi
	$(MAKE) -C /run/media/fabio/0880a32b-0f6f-457f-9e41-6a9139ce15b9/repos/ldd/linux-2.6 SUBDIRS=`pwd`  CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
	$(MAKE) -C /run/media/fabio/0880a32b-0f6f-457f-9e41-6a9139ce15b9/repos/ldd/linux-2.6 SUBDIRS=`pwd`  $@

	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /run/media/fabio/0880a32b-0f6f-457f-9e41-6a9139ce15b9/repos/ldd/linux-2.6 SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/zfs $@
	@# Remove extraneous build products when packaging
	if [ -n "$(DESTDIR)" ]; then \
		find $(DESTDIR)/lib/modules/3.5.2+ \
			-name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)/boot/System.map-3.5.2+; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 3.5.2+; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	$(RM) -R $(DESTDIR)/lib/modules/3.5.2+/addon/zfs

distdir:

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
