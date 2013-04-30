# Makefile.in for spl kernel module

MODULE := spl
EXTRA_CFLAGS = $(SPL_MODULE_CFLAGS)  -DHAVE_GPL_ONLY_SYMBOLS -Wstrict-prototypes -DNDEBUG -DDEBUG_LOG -DDEBUG_KMEM

# Solaris porting layer module
obj-$(CONFIG_SPL) := $(MODULE).o

$(MODULE)-objs += spl-debug.o
$(MODULE)-objs += spl-proc.o
$(MODULE)-objs += spl-kmem.o
$(MODULE)-objs += spl-thread.o
$(MODULE)-objs += spl-taskq.o
$(MODULE)-objs += spl-rwlock.o
$(MODULE)-objs += spl-vnode.o
$(MODULE)-objs += spl-err.o
$(MODULE)-objs += spl-time.o
$(MODULE)-objs += spl-kobj.o
$(MODULE)-objs += spl-generic.o
$(MODULE)-objs += spl-atomic.o
$(MODULE)-objs += spl-mutex.o
$(MODULE)-objs += spl-kstat.o
$(MODULE)-objs += spl-condvar.o
$(MODULE)-objs += spl-xdr.o
$(MODULE)-objs += spl-cred.o
$(MODULE)-objs += spl-tsd.o
$(MODULE)-objs += spl-zlib.o
