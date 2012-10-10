subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

ZFS_MODULE_CFLAGS += -include /home/fabio/repos/spl/spl_config.h
ZFS_MODULE_CFLAGS += -include /home/fabio/repos/zfs-upstream/zfs_config.h
ZFS_MODULE_CFLAGS += -I/home/fabio/repos/zfs-upstream/include -I/home/fabio/repos/spl/include -I/home/fabio/repos/spl
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /home/fabio/repos/spl/Module.symvers ]; then \
		/bin/cp /home/fabio/repos/spl/Module.symvers .; \
	elif [ -f /home/fabio/repos/spl/module/Module.symvers ]; then \
		/bin/cp /home/fabio/repos/spl/module/Module.symvers .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /home/fabio/repos/spl/Module.symvers, or\n" \
		"*** - /home/fabio/repos/spl/module/Module.symvers\n"; \
		exit 1; \
	fi
	$(MAKE) -C /home/fabio/repos/kernel SUBDIRS=`pwd`  CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
	$(MAKE) -C /home/fabio/repos/kernel SUBDIRS=`pwd`  $@

	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.symvers ]; then $(RM) Module.symvers; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /home/fabio/repos/kernel SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/zfs $@
	@# Remove extraneous build products when packaging
	if [ -n "$(DESTDIR)" ]; then \
		find $(DESTDIR)/lib/modules/3.6.0+ \
			-name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)/boot/System.map-3.6.0+; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 3.6.0+; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	$(RM) -R $(DESTDIR)/lib/modules/3.6.0+/addon/zfs

distdir:

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
