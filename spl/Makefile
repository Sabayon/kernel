subdir-m += spl
subdir-m += splat

INSTALL=/usr/bin/install

SPL_MODULE_CFLAGS  = -I/var/tmp/portage/sys-kernel/spl-9999/work/spl-9999/include
SPL_MODULE_CFLAGS += -include /var/tmp/portage/sys-kernel/spl-9999/work/spl-9999/spl_config.h
export SPL_MODULE_CFLAGS

modules:
	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd`  O=/lib/modules/3.5.0/build CONFIG_SPL=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
#	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd`  O=/lib/modules/3.5.0/build $@

	if [ -f 'NONE' ]; then $(RM) 'NONE'; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /lib/modules/3.5.0/build SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/spl $@
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
	$(RM) -R $(DESTDIR)/lib/modules/3.5.0/addon/spl

distdir:
	list='$(subdir-m)'; for subdir in $$list; do \
		(find ../module/$$subdir -name '*.c' -o -name '*.h' |\
		 xargs /bin/cp -t $$distdir/$$subdir); \
	done

distclean maintainer-clean: clean
install: modules_install
uninstall: modules_uninstall
all: modules
check:
