#ifndef __LINUX_COMPILER_H
#error "Please don't include <linux/compiler-gcc4.h> directly, include <linux/compiler.h> instead."
#endif

/* GCC 4.1.[01] miscompiles __weak */
#ifdef __KERNEL__
# if __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ <= 1
#  error Your version of gcc miscompiles the __weak directive
# endif
#endif

#define __used			__attribute__((__used__))
#define __must_check 		__attribute__((warn_unused_result))
#define __compiler_offsetof(a,b) __builtin_offsetof(a,b)

#if __GNUC_MINOR__ >= 3
/* Mark functions as cold. gcc will assume any path leading to a call
   to them will be unlikely.  This means a lot of manual unlikely()s
   are unnecessary now for any paths leading to the usual suspects
   like BUG(), printk(), panic() etc. [but let's keep them for now for
   older compilers]

   Early snapshots of gcc 4.3 don't support this and we can't detect this
   in the preprocessor, but we can live with this because they're unreleased.
   Maketime probing would be overkill here.

   gcc also has a __attribute__((__hot__)) to move hot functions into
   a special section, but I don't see any sense in this right now in
   the kernel context */
#define __cold			__attribute__((__cold__))

#define __linktime_error(message) __attribute__((__error__(message)))

#if __GNUC_MINOR__ >= 5

#ifdef CONSTIFY_PLUGIN
#define __no_const __attribute__((no_const))
#define __do_const __attribute__((do_const))
#endif

#ifdef SIZE_OVERFLOW_PLUGIN
#define __size_overflow(...) __attribute__((size_overflow(__VA_ARGS__)))
#define __intentional_overflow(...) __attribute__((intentional_overflow(__VA_ARGS__)))
#endif

#ifdef LATENT_ENTROPY_PLUGIN
#define __latent_entropy __attribute__((latent_entropy))
#endif

/*
 * Mark a position in code as unreachable.  This can be used to
 * suppress control flow warnings after asm blocks that transfer
 * control elsewhere.
 *
 * Early snapshots of gcc 4.5 don't support this and we can't detect
 * this in the preprocessor, but we can live with this because they're
 * unreleased.  Really, we need to have autoconf for the kernel.
 */
#define unreachable() __builtin_unreachable()

/* Mark a function definition as prohibited from being cloned. */
#define __noclone	__attribute__((__noclone__))

#endif

#define __alloc_size(...)	__attribute((alloc_size(__VA_ARGS__)))
#define __bos(ptr, arg)		__builtin_object_size((ptr), (arg))
#define __bos0(ptr)		__bos((ptr), 0)
#define __bos1(ptr)		__bos((ptr), 1)
#endif

#if __GNUC_MINOR__ >= 6
/*
 * Tell the optimizer that something else uses this function or variable.
 */
#define __visible __attribute__((externally_visible))
#endif

#if __GNUC_MINOR__ > 0
#define __compiletime_object_size(obj) __builtin_object_size(obj, 0)
#endif
#if __GNUC_MINOR__ >= 3 && !defined(__CHECKER__)
#define __compiletime_warning(message) __attribute__((warning(message)))
#define __compiletime_error(message) __attribute__((error(message)))
#endif
