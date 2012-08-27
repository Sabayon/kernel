/* zfs_config.h.  Generated from zfs_config.h.in by configure.  */
/* zfs_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 to enabled dmu tx validation */
/* #undef DEBUG_DMU_TX */

/* invalidate_bdev() wants 1 arg */
#define HAVE_1ARG_INVALIDATE_BDEV 1

/* bio_end_io_t wants 2 args */
#define HAVE_2ARGS_BIO_END_IO_T 1

/* blkdev_get() wants 3 args */
#define HAVE_3ARG_BLKDEV_GET 1

/* security_inode_init_security wants 6 args */
/* #undef HAVE_6ARGS_SECURITY_INODE_INIT_SECURITY */

/* dops->automount() exists */
#define HAVE_AUTOMOUNT 1

/* struct block_device_operations use bdevs */
#define HAVE_BDEV_BLOCK_DEVICE_OPERATIONS 1

/* bdev_logical_block_size() is available */
#define HAVE_BDEV_LOGICAL_BLOCK_SIZE 1

/* struct super_block has s_bdi */
#define HAVE_BDI 1

/* bdi_setup_and_register() is available */
#define HAVE_BDI_SETUP_AND_REGISTER 1

/* bio_empy_barrier() is defined */
/* #undef HAVE_BIO_EMPTY_BARRIER */

/* REQ_FAILFAST_MASK is defined */
#define HAVE_BIO_REQ_FAILFAST_MASK 1

/* BIO_RW_FAILFAST is defined */
/* #undef HAVE_BIO_RW_FAILFAST */

/* BIO_RW_FAILFAST_* are defined */
/* #undef HAVE_BIO_RW_FAILFAST_DTD */

/* BIO_RW_SYNC is defined */
/* #undef HAVE_BIO_RW_SYNC */

/* BIO_RW_SYNCIO is defined */
/* #undef HAVE_BIO_RW_SYNCIO */

/* blkdev_get_by_path() is available */
#define HAVE_BLKDEV_GET_BY_PATH 1

/* struct queue_limits with discard_zeroes_data */
#define HAVE_BLK_DEV_DISCARD_ZEROES_DATA 1

/* blk_end_request() is available */
#define HAVE_BLK_END_REQUEST 1

/* blk_end_request() is GPL-only */
/* #undef HAVE_BLK_END_REQUEST_GPL_ONLY */

/* blk_fetch_request() is available */
#define HAVE_BLK_FETCH_REQUEST 1

/* blk_queue_discard() is available */
#define HAVE_BLK_QUEUE_DISCARD 1

/* blk_queue_flush() is available */
#define HAVE_BLK_QUEUE_FLUSH 1

/* blk_queue_flush() is GPL-only */
/* #undef HAVE_BLK_QUEUE_FLUSH_GPL_ONLY */

/* blk_queue_io_opt() is available */
#define HAVE_BLK_QUEUE_IO_OPT 1

/* blk_queue_max_hw_sectors() is available */
#define HAVE_BLK_QUEUE_MAX_HW_SECTORS 1

/* blk_queue_max_segments() is available */
#define HAVE_BLK_QUEUE_MAX_SEGMENTS 1

/* blk_queue_nonrot() is available */
#define HAVE_BLK_QUEUE_NONROT 1

/* blk_queue_physical_block_size() is available */
#define HAVE_BLK_QUEUE_PHYSICAL_BLOCK_SIZE 1

/* blk_requeue_request() is available */
#define HAVE_BLK_REQUEUE_REQUEST 1

/* blk_rq_bytes() is available */
#define HAVE_BLK_RQ_BYTES 1

/* blk_rq_bytes() is GPL-only */
/* #undef HAVE_BLK_RQ_BYTES_GPL_ONLY */

/* blk_rq_pos() is available */
#define HAVE_BLK_RQ_POS 1

/* blk_rq_sectors() is available */
#define HAVE_BLK_RQ_SECTORS 1

/* security_inode_init_security wants callback */
#define HAVE_CALLBACK_SECURITY_INODE_INIT_SECURITY 1

/* check_disk_size_change() is available */
#define HAVE_CHECK_DISK_SIZE_CHANGE 1

/* clear_inode() is available */
#define HAVE_CLEAR_INODE 1

/* super_block uses const struct xattr_hander */
#define HAVE_CONST_XATTR_HANDLER 1

/* iops->create()/mkdir()/mknod() take umode_t */
#define HAVE_CREATE_UMODE_T 1

/* xattr_handler->get() wants dentry */
#define HAVE_DENTRY_XATTR_GET 1

/* xattr_handler->set() wants dentry */
#define HAVE_DENTRY_XATTR_SET 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* d_make_root() is available */
#define HAVE_D_MAKE_ROOT 1

/* d_obtain_alias() is available */
#define HAVE_D_OBTAIN_ALIAS 1

/* eops->encode_fh() wants child and parent inodes */
#define HAVE_ENCODE_FH_WITH_INODE 1

/* sops->evict_inode() exists */
#define HAVE_EVICT_INODE 1

/* fops->fallocate() exists */
#define HAVE_FILE_FALLOCATE 1

/* kernel defines fmode_t */
#define HAVE_FMODE_T 1

/* sops->free_cached_objects() exists */
#define HAVE_FREE_CACHED_OBJECTS 1

/* fops->fsync() with range */
#define HAVE_FSYNC_RANGE 1

/* fops->fsync() without dentry */
/* #undef HAVE_FSYNC_WITHOUT_DENTRY */

/* fops->fsync() with dentry */
/* #undef HAVE_FSYNC_WITH_DENTRY */

/* blk_disk_ro() is available */
#define HAVE_GET_DISK_RO 1

/* get_gendisk() is available */
#define HAVE_GET_GENDISK 1

/* Define to 1 if licensed under the GPL */
/* #undef HAVE_GPL_ONLY_SYMBOLS */

/* fops->fallocate() exists */
/* #undef HAVE_INODE_FALLOCATE */

/* iops->truncate_range() exists */
/* #undef HAVE_INODE_TRUNCATE_RANGE */

/* insert_inode_locked() is available */
#define HAVE_INSERT_INODE_LOCKED 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* result=stropts.h Define to 1 if ioctl() defined in <stropts.h> */
/* #undef HAVE_IOCTL_IN_STROPTS_H */

/* Define to 1 if ioctl() defined in <sys/ioctl.h> */
#define HAVE_IOCTL_IN_SYS_IOCTL_H 1

/* Define to 1 if ioctl() defined in <unistd.h> */
/* #undef HAVE_IOCTL_IN_UNISTD_H */

/* kernel defines KOBJ_NAME_LEN */
/* #undef HAVE_KOBJ_NAME_LEN */

/* Define if you have libblkid */
/* #undef HAVE_LIBBLKID */

/* Define if you have selinux */
/* #undef HAVE_LIBSELINUX */

/* Define if you have libuuid */
#define HAVE_LIBUUID 1

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* mount_nodev() is available */
#define HAVE_MOUNT_NODEV 1

/* sops->nr_cached_objects() exists */
#define HAVE_NR_CACHED_OBJECTS 1

/* open_bdev_exclusive() is available */
/* #undef HAVE_OPEN_BDEV_EXCLUSIVE */

/* REQ_SYNC is defined */
#define HAVE_REQ_SYNC 1

/* rq_for_each_segment() is available */
#define HAVE_RQ_FOR_EACH_SEGMENT 1

/* rq_is_sync() is available */
#define HAVE_RQ_IS_SYNC 1

/* set_nlink() is available */
#define HAVE_SET_NLINK 1

/* sops->show_options() with dentry */
#define HAVE_SHOW_OPTIONS_WITH_DENTRY 1

/* struct super_block has s_shrink */
#define HAVE_SHRINK 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* truncate_setsize() is available */
#define HAVE_TRUNCATE_SETSIZE 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you have zlib */
#define HAVE_ZLIB 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if NPTL threading implementation includes guard area in stack
   allocation */
/* #undef NPTL_GUARD_WITHIN_STACK */

/* zfs debugging enabled */
/* #undef ZFS_DEBUG */

/* Define the project alias string. */
#define ZFS_META_ALIAS "zfs-0.6.0-rc9"

/* Define the project author. */
#define ZFS_META_AUTHOR "Sun Microsystems/Oracle, Lawrence Livermore National Laboratory"

/* Define the project release date. */
/* #undef ZFS_META_DATA */

/* Define the project license. */
#define ZFS_META_LICENSE "CDDL"

/* Define the libtool library 'age' version information. */
/* #undef ZFS_META_LT_AGE */

/* Define the libtool library 'current' version information. */
/* #undef ZFS_META_LT_CURRENT */

/* Define the libtool library 'revision' version information. */
/* #undef ZFS_META_LT_REVISION */

/* Define the project name. */
#define ZFS_META_NAME "zfs"

/* Define the project release. */
#define ZFS_META_RELEASE "rc9"

/* Define the project version. */
#define ZFS_META_VERSION "0.6.0"

