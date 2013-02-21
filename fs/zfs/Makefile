subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

ZFS_MODULE_CFLAGS += -include /home/fabio/repos-local/spl/spl_config.h
ZFS_MODULE_CFLAGS += -include /home/fabio/repos-local/zfs/zfs_config.h
ZFS_MODULE_CFLAGS += -I/home/fabio/repos-local/zfs/include -I/home/fabio/repos-local/spl/include -I/home/fabio/repos-local/spl
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /home/fabio/repos-local/spl/NONE ]; then \
		/bin/cp /home/fabio/repos-local/spl/NONE .; \
	elif [ -f /home/fabio/repos-local/spl/module/NONE ]; then \
		/bin/cp /home/fabio/repos-local/spl/module/NONE .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /home/fabio/repos-local/spl/NONE, or\n" \
		"*** - /home/fabio/repos-local/spl/module/NONE\n"; \
		exit 1; \
	fi
	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd`  CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
#	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd`  $@

	if [ -f NONE ]; then $(RM) NONE; fi
	if [ -f NONE ]; then $(RM) NONE; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/zfs $@
	@# Remove extraneous build products when packaging
	if [ -n "$(DESTDIR)" ]; then \
		find $(DESTDIR)/lib/modules/3.8.0+ \
			-name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)/boot/System.map-3.8.0+; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 3.8.0+; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	$(RM) -R $(DESTDIR)/lib/modules/3.8.0+/addon/zfs

distdir:

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
