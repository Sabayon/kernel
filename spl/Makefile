subdir-m += spl
subdir-m += splat

INSTALL=/usr/bin/install

SPL_MODULE_CFLAGS  = -I/home/fabio/repos-local/spl/include
SPL_MODULE_CFLAGS += -include /home/fabio/repos-local/spl/spl_config.h
export SPL_MODULE_CFLAGS

modules:
	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd`  CONFIG_SPL=m $@

clean:
	@# Only cleanup the kernel build directories when CONFIG_KERNEL
	@# is defined.  This indicates that kernel modules should be built.
#	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd`  $@

	if [ -f 'NONE' ]; then $(RM) 'NONE'; fi
	if [ -f Module.markers ]; then $(RM) Module.markers; fi

modules_install:
	@# Install the kernel modules
	$(MAKE) -C /home/fabio/repos-local/sabayon-kernel SUBDIRS=`pwd` \
		INSTALL_MOD_PATH=$(DESTDIR) \
		INSTALL_MOD_DIR=addon/spl $@
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
	$(RM) -R $(DESTDIR)/lib/modules/3.8.0+/addon/spl

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
