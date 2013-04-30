subdir-m += avl
subdir-m += nvpair
subdir-m += unicode
subdir-m += zcommon
subdir-m += zfs
subdir-m += zpios

INSTALL_MOD_DIR ?= extra

ZFS_MODULE_CFLAGS += -include /home/fabio/repos/spl/spl_config.h
ZFS_MODULE_CFLAGS += -include /home/fabio/repos/zfs/zfs_config.h
ZFS_MODULE_CFLAGS += -I/home/fabio/repos/zfs/include -I/home/fabio/repos/spl/include -I/home/fabio/repos/spl
export ZFS_MODULE_CFLAGS

modules:
	@# Make the exported SPL symbols available to these modules.
	@# They may be in the root of SPL_OBJ when building against
	@# installed devel headers, or they may be in the module
	@# subdirectory when building against the spl source tree.
	@if [ -f /home/fabio/repos/spl/NONE ]; then \
		/bin/cp /home/fabio/repos/spl/NONE .; \
	elif [ -f /home/fabio/repos/spl/module/NONE ]; then \
		/bin/cp /home/fabio/repos/spl/module/NONE .; \
	else \
		echo -e "\n" \
		"*** Missing spl symbols ensure you have built the spl:\n" \
		"*** - /home/fabio/repos/spl/NONE, or\n" \
		"*** - /home/fabio/repos/spl/module/NONE\n"; \
		exit 1; \
	fi
	$(MAKE) -C /home/fabio/repos/sabayon-2.6 SUBDIRS=`pwd`  CONFIG_ZFS=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
#	$(MAKE) -C /home/fabio/repos/sabayon-2.6 SUBDIRS=`pwd`  $@

	if [ -f NONE ]; then $(RM) NONE; fi
	if [ -f NONE ]; then $(RM) NONE; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /home/fabio/repos/sabayon-2.6 SUBDIRS=`pwd` $@ \
		INSTALL_MOD_PATH=$(DESTDIR)$(INSTALL_MOD_PATH) \
		INSTALL_MOD_DIR=$(INSTALL_MOD_DIR)
	@# Remove extraneous build products when packaging
	kmoddir=$(DESTDIR)$(INSTALL_MOD_PATH)/lib/modules/3.8.5+; \
	if [ -n $$kmoddir ]; then \
		find $$kmoddir -name 'modules.*' | xargs $(RM); \
	fi
	sysmap=$(DESTDIR)$(INSTALL_MOD_PATH)/boot/System.map-3.8.5+; \
	if [ -f $$sysmap ]; then \
		depmod -ae -F $$sysmap 3.8.5+; \
	fi

modules_uninstall:
	@# Uninstall the kernel modules
	kmoddir=$(DESTDIR)$(INSTALL_MOD_PATH)/lib/modules/3.8.5+
	list='$(subdir-m)'; for subdir in $$list; do \
		$(RM) -R $$kmoddir/$(INSTALL_MOD_DIR)/$$subdir; \
	done

distdir:

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
