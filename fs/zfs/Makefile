subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

ZFS_MODULE_CFLAGS += -include /usr/src/spl-0.6.0-rc9/3.5.0/spl_config.h
ZFS_MODULE_CFLAGS += -include /var/tmp/portage/sys-fs/zfs-9999/work/zfs-9999/zfs_config.h
ZFS_MODULE_CFLAGS += -I/var/tmp/portage/sys-fs/zfs-9999/work/zfs-9999/include -I/usr/src/spl-0.6.0-rc9/3.5.0/include -I/usr/src/spl-0.6.0-rc9/3.5.0
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /usr/src/spl-0.6.0-rc9/3.5.0/Module.symvers ]; then \
		/bin/cp /usr/src/spl-0.6.0-rc9/3.5.0/Module.symvers .; \
	elif [ -f /usr/src/spl-0.6.0-rc9/3.5.0/module/Module.symvers ]; then \
		/bin/cp /usr/src/spl-0.6.0-rc9/3.5.0/module/Module.symvers .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /usr/src/spl-0.6.0-rc9/3.5.0/Module.symvers, or\n" \
		"*** - /usr/src/spl-0.6.0-rc9/3.5.0/module/Module.symvers\n"; \
		exit 1; \
	fi
	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd`  O=/lib/modules/3.5.0/build CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
#	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd`  O=/lib/modules/3.5.0/build $@

	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f NONE ]; then $(RM) NONE; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/zfs $@
	@# Remove extraneous build products when packaging
	if [ -n "$(DESTDIR)" ]; then \
		find $(DESTDIR)/lib/modules/3.5.0 \
			-name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)/boot/System.map-3.5.0; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 3.5.0; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	$(RM) -R $(DESTDIR)/lib/modules/3.5.0/addon/zfs

distdir:

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
