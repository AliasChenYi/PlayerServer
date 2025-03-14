#ifndef JEMALLOC_INTERNAL_H
#define JEMALLOC_INTERNAL_H
#include <math.h>
#ifdef _WIN32
#  include <windows.h>
#  ifdef ENOENT
#  undef ENOENT
#  endif
#  define ENOENT ERROR_PATH_NOT_FOUND
#  ifndef EINVAL
#  define EINVAL ERROR_BAD_ARGUMENTS
#  endif
#  ifndef EAGAIN
#  define EAGAIN ERROR_OUTOFMEMORY
#  endif
#  ifndef EPERM
#  define EPERM  ERROR_WRITE_FAULT
#  endif
#  ifndef EFAULT
#  define EFAULT ERROR_INVALID_ADDRESS
#  endif
#  ifndef ENOMEM
#  define ENOMEM ERROR_NOT_ENOUGH_MEMORY
#  endif
#  undef ERANGE
#  define ERANGE ERROR_INVALID_DATA
#else
#  include <sys/param.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  if !defined(SYS_write) && defined(__NR_write)
#    define SYS_write __NR_write
#  endif
#  include <sys/uio.h>
#  include <pthread.h>
#  include <errno.h>
#endif
#include <sys/types.h>

#include <limits.h>
#ifndef SIZE_T_MAX
#  define SIZE_T_MAX	SIZE_MAX
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#ifndef offsetof
#  define offsetof(type, member)	((size_t)&(((type *)NULL)->member))
#endif
#include <inttypes.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#ifdef _MSC_VER
#  include <io.h>
typedef intptr_t ssize_t;
#  define PATH_MAX 1024
#  define STDERR_FILENO 2
#  define __func__ __FUNCTION__
/* Disable warnings about deprecated system functions */
#  pragma warning(disable: 4996)
#else
#  include <unistd.h>
#endif
#include <fcntl.h>

#define	JEMALLOC_NO_DEMANGLE
#include "../jemalloc.h"

#ifdef JEMALLOC_UTRACE
#include <sys/ktrace.h>
#endif

#ifdef JEMALLOC_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#include "jemalloc/internal/private_namespace.h"

#ifdef JEMALLOC_CC_SILENCE
#define	UNUSED JEMALLOC_ATTR(unused)
#else
#define	UNUSED
#endif

static const bool config_debug =
#ifdef JEMALLOC_DEBUG
    true
#else
    false
#endif
    ;
static const bool config_dss =
#ifdef JEMALLOC_DSS
    true
#else
    false
#endif
    ;
static const bool config_fill =
#ifdef JEMALLOC_FILL
    true
#else
    false
#endif
    ;
static const bool config_lazy_lock =
#ifdef JEMALLOC_LAZY_LOCK
    true
#else
    false
#endif
    ;
static const bool config_prof =
#ifdef JEMALLOC_PROF
    true
#else
    false
#endif
    ;
static const bool config_prof_libgcc =
#ifdef JEMALLOC_PROF_LIBGCC
    true
#else
    false
#endif
    ;
static const bool config_prof_libunwind =
#ifdef JEMALLOC_PROF_LIBUNWIND
    true
#else
    false
#endif
    ;
static const bool config_mremap =
#ifdef JEMALLOC_MREMAP
    true
#else
    false
#endif
    ;
static const bool config_munmap =
#ifdef JEMALLOC_MUNMAP
    true
#else
    false
#endif
    ;
static const bool config_stats =
#ifdef JEMALLOC_STATS
    true
#else
    false
#endif
    ;
static const bool config_tcache =
#ifdef JEMALLOC_TCACHE
    true
#else
    false
#endif
    ;
static const bool config_tls =
#ifdef JEMALLOC_TLS
    true
#else
    false
#endif
    ;
static const bool config_utrace =
#ifdef JEMALLOC_UTRACE
    true
#else
    false
#endif
    ;
static const bool config_valgrind =
#ifdef JEMALLOC_VALGRIND
    true
#else
    false
#endif
    ;
static const bool config_xmalloc =
#ifdef JEMALLOC_XMALLOC
    true
#else
    false
#endif
    ;
static const bool config_ivsalloc =
#ifdef JEMALLOC_IVSALLOC
    true
#else
    false
#endif
    ;

#ifdef JEMALLOC_ATOMIC9
#include <machine/atomic.h>
#endif

#if (defined(JEMALLOC_OSATOMIC) || defined(JEMALLOC_OSSPIN))
#include <libkern/OSAtomic.h>
#endif

#ifdef JEMALLOC_ZONE
#include <mach/mach_error.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <malloc/malloc.h>
#endif

#define	RB_COMPACT
#include "jemalloc/internal/rb.h"
#include "jemalloc/internal/qr.h"
#include "jemalloc/internal/ql.h"

/*
 * jemalloc can conceptually be broken into components (arena, tcache, etc.),
 * but there are circular dependencies that cannot be broken without
 * substantial performance degradation.  In order to reduce the effect on
 * visual code flow, read the header files in multiple passes, with one of the
 * following cpp variables defined during each pass:
 *
 *   JEMALLOC_H_TYPES   : Preprocessor-defined constants and psuedo-opaque data
 *                        types.
 *   JEMALLOC_H_STRUCTS : Data structures.
 *   JEMALLOC_H_EXTERNS : Extern data declarations and function prototypes.
 *   JEMALLOC_H_INLINES : Inline functions.
 */
/******************************************************************************/
#define JEMALLOC_H_TYPES

#define	ALLOCM_LG_ALIGN_MASK	((int)0x3f)

#define	ZU(z)	((size_t)z)
#define	QU(q)	((uint64_t)q)

#ifndef __DECONST
#  define	__DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif

#ifdef JEMALLOC_DEBUG
   /* Disable inlining to make debugging easier. */
#  define JEMALLOC_ALWAYS_INLINE
#  define JEMALLOC_INLINE
#  define inline
#else
#  define JEMALLOC_ENABLE_INLINE
#  ifdef JEMALLOC_HAVE_ATTR
#    define JEMALLOC_ALWAYS_INLINE \
	 static inline JEMALLOC_ATTR(unused) JEMALLOC_ATTR(always_inline)
#  else
#    define JEMALLOC_ALWAYS_INLINE static inline
#  endif
#  define JEMALLOC_INLINE static inline
#  ifdef _MSC_VER
#    define inline _inline
#  endif
#endif

/* Smallest size class to support. */
#define	LG_TINY_MIN		3
#define	TINY_MIN		(1U << LG_TINY_MIN)

/*
 * Minimum alignment of allocations is 2^LG_QUANTUM bytes (ignoring tiny size
 * classes).
 */
#ifndef LG_QUANTUM
#  if (defined(__i386__) || defined(_M_IX86))
#    define LG_QUANTUM		4
#  endif
#  ifdef __ia64__
#    define LG_QUANTUM		4
#  endif
#  ifdef __alpha__
#    define LG_QUANTUM		4
#  endif
#  ifdef __sparc64__
#    define LG_QUANTUM		4
#  endif
#  if (defined(__amd64__) || defined(__x86_64__) || defined(_M_X64))
#    define LG_QUANTUM		4
#  endif
#  ifdef __arm__
#    define LG_QUANTUM		3
#  endif
#  ifdef __hppa__
#    define LG_QUANTUM		4
#  endif
#  ifdef __mips__
#    define LG_QUANTUM		3
#  endif
#  ifdef __powerpc__
#    define LG_QUANTUM		4
#  endif
#  ifdef __s390__
#    define LG_QUANTUM		4
#  endif
#  ifdef __SH4__
#    define LG_QUANTUM		4
#  endif
#  ifdef __tile__
#    define LG_QUANTUM		4
#  endif
#  ifndef LG_QUANTUM
#    error "No LG_QUANTUM definition for architecture; specify via CPPFLAGS"
#  endif
#endif

#define	QUANTUM			((size_t)(1U << LG_QUANTUM))
#define	QUANTUM_MASK		(QUANTUM - 1)

/* Return the smallest quantum multiple that is >= a. */
#define	QUANTUM_CEILING(a)						\
	(((a) + QUANTUM_MASK) & ~QUANTUM_MASK)

#define	LONG			((size_t)(1U << LG_SIZEOF_LONG))
#define	LONG_MASK		(LONG - 1)

/* Return the smallest long multiple that is >= a. */
#define	LONG_CEILING(a)							\
	(((a) + LONG_MASK) & ~LONG_MASK)

#define	SIZEOF_PTR		(1U << LG_SIZEOF_PTR)
#define	PTR_MASK		(SIZEOF_PTR - 1)

/* Return the smallest (void *) multiple that is >= a. */
#define	PTR_CEILING(a)							\
	(((a) + PTR_MASK) & ~PTR_MASK)

/*
 * Maximum size of L1 cache line.  This is used to avoid cache line aliasing.
 * In addition, this controls the spacing of cacheline-spaced size classes.
 *
 * CACHELINE cannot be based on LG_CACHELINE because __declspec(align()) can
 * only handle raw constants.
 */
#define	LG_CACHELINE		6
#define	CACHELINE		64
#define	CACHELINE_MASK		(CACHELINE - 1)

/* Return the smallest cacheline multiple that is >= s. */
#define	CACHELINE_CEILING(s)						\
	(((s) + CACHELINE_MASK) & ~CACHELINE_MASK)

/* Page size.  STATIC_PAGE_SHIFT is determined by the configure script. */
#ifdef PAGE_MASK
#  undef PAGE_MASK
#endif
#define	LG_PAGE		STATIC_PAGE_SHIFT
#define	PAGE		((size_t)(1U << STATIC_PAGE_SHIFT))
#define	PAGE_MASK	((size_t)(PAGE - 1))

/* Return the smallest pagesize multiple that is >= s. */
#define	PAGE_CEILING(s)							\
	(((s) + PAGE_MASK) & ~PAGE_MASK)

/* Return the nearest aligned address at or below a. */
#define	ALIGNMENT_ADDR2BASE(a, alignment)				\
	((void *)((uintptr_t)(a) & (-(alignment))))

/* Return the offset between a and the nearest aligned address at or below a. */
#define	ALIGNMENT_ADDR2OFFSET(a, alignment)				\
	((size_t)((uintptr_t)(a) & (alignment - 1)))

/* Return the smallest alignment multiple that is >= s. */
#define	ALIGNMENT_CEILING(s, alignment)					\
	(((s) + (alignment - 1)) & (-(alignment)))

/* Declare a variable length array */
#if __STDC_VERSION__ < 199901L
#  ifdef _MSC_VER
#    include <malloc.h>
#    define alloca _alloca
#  else
#    ifdef JEMALLOC_HAS_ALLOCA_H
#      include <alloca.h>
#    else
#      include <stdlib.h>
#    endif
#  endif
#  define VARIABLE_ARRAY(type, name, count) \
	type *name = alloca(sizeof(type) * count)
#else
#  define VARIABLE_ARRAY(type, name, count) type name[count]
#endif

#ifdef JEMALLOC_VALGRIND
/*
 * The JEMALLOC_VALGRIND_*() macros must be macros rather than functions
 * so that when Valgrind reports errors, there are no extra stack frames
 * in the backtraces.
 *
 * The size that is reported to valgrind must be consistent through a chain of
 * malloc..realloc..realloc calls.  Request size isn't recorded anywhere in
 * jemalloc, so it is critical that all callers of these macros provide usize
 * rather than request size.  As a result, buffer overflow detection is
 * technically weakened for the standard API, though it is generally accepted
 * practice to consider any extra bytes reported by malloc_usable_size() as
 * usable space.
 */
#define	JEMALLOC_VALGRIND_MALLOC(cond, ptr, usize, zero) do {		\
	if (config_valgrind && opt_valgrind && cond)			\
		VALGRIND_MALLOCLIKE_BLOCK(ptr, usize, p2rz(ptr), zero);	\
} while (0)
#define	JEMALLOC_VALGRIND_REALLOC(ptr, usize, old_ptr, old_usize,	\
    old_rzsize, zero)  do {						\
	if (config_valgrind && opt_valgrind) {				\
		size_t rzsize = p2rz(ptr);				\
									\
		if (ptr == old_ptr) {					\
			VALGRIND_RESIZEINPLACE_BLOCK(ptr, old_usize,	\
			    usize, rzsize);				\
			if (zero && old_usize < usize) {		\
				VALGRIND_MAKE_MEM_DEFINED(		\
				    (void *)((uintptr_t)ptr +		\
				    old_usize), usize - old_usize);	\
			}						\
		} else {						\
			if (old_ptr != NULL) {				\
				VALGRIND_FREELIKE_BLOCK(old_ptr,	\
				    old_rzsize);			\
			}						\
			if (ptr != NULL) {				\
				size_t copy_size = (old_usize < usize)	\
				    ?  old_usize : usize;		\
				size_t tail_size = usize - copy_size;	\
				VALGRIND_MALLOCLIKE_BLOCK(ptr, usize,	\
				    rzsize, false);			\
				if (copy_size > 0) {			\
					VALGRIND_MAKE_MEM_DEFINED(ptr,	\
					    copy_size);			\
				}					\
				if (zero && tail_size > 0) {		\
					VALGRIND_MAKE_MEM_DEFINED(	\
					    (void *)((uintptr_t)ptr +	\
					    copy_size), tail_size);	\
				}					\
			}						\
		}							\
	}								\
} while (0)
#define	JEMALLOC_VALGRIND_FREE(ptr, rzsize) do {			\
	if (config_valgrind && opt_valgrind)				\
		VALGRIND_FREELIKE_BLOCK(ptr, rzsize);			\
} while (0)
#else
#define	RUNNING_ON_VALGRIND	((unsigned)0)
#define	VALGRIND_MALLOCLIKE_BLOCK(addr, sizeB, rzB, is_zeroed) \
    do {} while (0)
#define	VALGRIND_RESIZEINPLACE_BLOCK(addr, oldSizeB, newSizeB, rzB) \
    do {} while (0)
#define	VALGRIND_FREELIKE_BLOCK(addr, rzB) do {} while (0)
#define	VALGRIND_MAKE_MEM_NOACCESS(_qzz_addr, _qzz_len) do {} while (0)
#define	VALGRIND_MAKE_MEM_UNDEFINED(_qzz_addr, _qzz_len) do {} while (0)
#define	VALGRIND_MAKE_MEM_DEFINED(_qzz_addr, _qzz_len) do {} while (0)
#define	JEMALLOC_VALGRIND_MALLOC(cond, ptr, usize, zero) do {} while (0)
#define	JEMALLOC_VALGRIND_REALLOC(ptr, usize, old_ptr, old_usize,	\
    old_rzsize, zero) do {} while (0)
#define	JEMALLOC_VALGRIND_FREE(ptr, rzsize) do {} while (0)
#endif

#include "jemalloc/internal/util.h"
#include "jemalloc/internal/atomic.h"
#include "jemalloc/internal/prng.h"
#include "jemalloc/internal/ckh.h"
#include "jemalloc/internal/size_classes.h"
#include "jemalloc/internal/stats.h"
#include "jemalloc/internal/ctl.h"
#include "jemalloc/internal/mutex.h"
#include "jemalloc/internal/tsd.h"
#include "jemalloc/internal/mb.h"
#include "jemalloc/internal/extent.h"
#include "jemalloc/internal/arena.h"
#include "jemalloc/internal/bitmap.h"
#include "jemalloc/internal/base.h"
#include "jemalloc/internal/chunk.h"
#include "jemalloc/internal/huge.h"
#include "jemalloc/internal/rtree.h"
#include "jemalloc/internal/tcache.h"
#include "jemalloc/internal/hash.h"
#include "jemalloc/internal/quarantine.h"
#include "jemalloc/internal/prof.h"

#undef JEMALLOC_H_TYPES
/******************************************************************************/
#define JEMALLOC_H_STRUCTS

#include "jemalloc/internal/util.h"
#include "jemalloc/internal/atomic.h"
#include "jemalloc/internal/prng.h"
#include "jemalloc/internal/ckh.h"
#include "jemalloc/internal/size_classes.h"
#include "jemalloc/internal/stats.h"
#include "jemalloc/internal/ctl.h"
#include "jemalloc/internal/mutex.h"
#include "jemalloc/internal/tsd.h"
#include "jemalloc/internal/mb.h"
#include "jemalloc/internal/bitmap.h"
#include "jemalloc/internal/extent.h"
#include "jemalloc/internal/arena.h"
#include "jemalloc/internal/base.h"
#include "jemalloc/internal/chunk.h"
#include "jemalloc/internal/huge.h"
#include "jemalloc/internal/rtree.h"
#include "jemalloc/internal/tcache.h"
#include "jemalloc/internal/hash.h"
#include "jemalloc/internal/quarantine.h"
#include "jemalloc/internal/prof.h"

typedef struct {
	uint64_t	allocated;
	uint64_t	deallocated;
} thread_allocated_t;
/*
 * The JEMALLOC_CONCAT() wrapper is necessary to pass {0, 0} via a cpp macro
 * argument.
 */
#define	THREAD_ALLOCATED_INITIALIZER	JEMALLOC_CONCAT({0, 0})

#undef JEMALLOC_H_STRUCTS
/******************************************************************************/
#define JEMALLOC_H_EXTERNS

extern bool	opt_abort;
extern bool	opt_junk;
extern size_t	opt_quarantine;
extern bool	opt_redzone;
extern bool	opt_utrace;
extern bool	opt_valgrind;
extern bool	opt_xmalloc;
extern bool	opt_zero;
extern size_t	opt_narenas;

/* Number of CPUs. */
extern unsigned		ncpus;

/* Protects arenas initialization (arenas, arenas_total). */
extern malloc_mutex_t	arenas_lock;
/*
 * Arenas that are used to service external requests.  Not all elements of the
 * arenas array are necessarily used; arenas are created lazily as needed.
 *
 * arenas[0..narenas_auto) are used for automatic multiplexing of threads and
 * arenas.  arenas[narenas_auto..narenas_total) are only used if the application
 * takes some action to create them and allocate from them.
 */
extern arena_t		**arenas;
extern unsigned		narenas_total;
extern unsigned		narenas_auto; /* Read-only after initialization. */

arena_t	*arenas_extend(unsigned ind);
void	arenas_cleanup(void *arg);
arena_t	*choose_arena_hard(void);
void	jemalloc_prefork(void);
void	jemalloc_postfork_parent(void);
void	jemalloc_postfork_child(void);

#include "jemalloc/internal/util.h"
#include "jemalloc/internal/atomic.h"
#include "jemalloc/internal/prng.h"
#include "jemalloc/internal/ckh.h"
#include "jemalloc/internal/size_classes.h"
#include "jemalloc/internal/stats.h"
#include "jemalloc/internal/ctl.h"
#include "jemalloc/internal/mutex.h"
#include "jemalloc/internal/tsd.h"
#include "jemalloc/internal/mb.h"
#include "jemalloc/internal/bitmap.h"
#include "jemalloc/internal/extent.h"
#include "jemalloc/internal/arena.h"
#include "jemalloc/internal/base.h"
#include "jemalloc/internal/chunk.h"
#include "jemalloc/internal/huge.h"
#include "jemalloc/internal/rtree.h"
#include "jemalloc/internal/tcache.h"
#include "jemalloc/internal/hash.h"
#include "jemalloc/internal/quarantine.h"
#include "jemalloc/internal/prof.h"

#undef JEMALLOC_H_EXTERNS
/******************************************************************************/
#define JEMALLOC_H_INLINES

#include "jemalloc/internal/util.h"
#include "jemalloc/internal/atomic.h"
#include "jemalloc/internal/prng.h"
#include "jemalloc/internal/ckh.h"
#include "jemalloc/internal/size_classes.h"
#include "jemalloc/internal/stats.h"
#include "jemalloc/internal/ctl.h"
#include "jemalloc/internal/mutex.h"
#include "jemalloc/internal/tsd.h"
#include "jemalloc/internal/mb.h"
#include "jemalloc/internal/extent.h"
#include "jemalloc/internal/base.h"
#include "jemalloc/internal/chunk.h"
#include "jemalloc/internal/huge.h"

#ifndef JEMALLOC_ENABLE_INLINE
malloc_tsd_protos(JEMALLOC_ATTR(unused), arenas, arena_t *)

size_t	s2u(size_t size);
size_t	sa2u(size_t size, size_t alignment);
unsigned	narenas_total_get(void);
arena_t	*choose_arena(arena_t *arena);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_C_))
/*
 * Map of pthread_self() --> arenas[???], used for selecting an arena to use
 * for allocations.
 */
malloc_tsd_externs(arenas, arena_t *)
malloc_tsd_funcs(JEMALLOC_ALWAYS_INLINE, arenas, arena_t *, NULL,
    arenas_cleanup)

/*
 * Compute usable size that would result from allocating an object with the
 * specified size.
 */
JEMALLOC_ALWAYS_INLINE size_t
s2u(size_t size)
{

	if (size <= SMALL_MAXCLASS)
		return (arena_bin_info[SMALL_SIZE2BIN(size)].reg_size);
	if (size <= arena_maxclass)
		return (PAGE_CEILING(size));
	return (CHUNK_CEILING(size));
}

/*
 * Compute usable size that would result from allocating an object with the
 * specified size and alignment.
 */
JEMALLOC_ALWAYS_INLINE size_t
sa2u(size_t size, size_t alignment)
{
	size_t usize;

	assert(alignment != 0 && ((alignment - 1) & alignment) == 0);

	/*
	 * Round size up to the nearest multiple of alignment.
	 *
	 * This done, we can take advantage of the fact that for each small
	 * size class, every object is aligned at the smallest power of two
	 * that is non-zero in the base two representation of the size.  For
	 * example:
	 *
	 *   Size |   Base 2 | Minimum alignment
	 *   -----+----------+------------------
	 *     96 |  1100000 |  32
	 *    144 | 10100000 |  32
	 *    192 | 11000000 |  64
	 */
	usize = ALIGNMENT_CEILING(size, alignment);
	/*
	 * (usize < size) protects against the combination of maximal
	 * alignment and size greater than maximal alignment.
	 */
	if (usize < size) {
		/* size_t overflow. */
		return (0);
	}

	if (usize <= arena_maxclass && alignment <= PAGE) {
		if (usize <= SMALL_MAXCLASS)
			return (arena_bin_info[SMALL_SIZE2BIN(usize)].reg_size);
		return (PAGE_CEILING(usize));
	} else {
		size_t run_size;

		/*
		 * We can't achieve subpage alignment, so round up alignment
		 * permanently; it makes later calculations simpler.
		 */
		alignment = PAGE_CEILING(alignment);
		usize = PAGE_CEILING(size);
		/*
		 * (usize < size) protects against very large sizes within
		 * PAGE of SIZE_T_MAX.
		 *
		 * (usize + alignment < usize) protects against the
		 * combination of maximal alignment and usize large enough
		 * to cause overflow.  This is similar to the first overflow
		 * check above, but it needs to be repeated due to the new
		 * usize value, which may now be *equal* to maximal
		 * alignment, whereas before we only detected overflow if the
		 * original size was *greater* than maximal alignment.
		 */
		if (usize < size || usize + alignment < usize) {
			/* size_t overflow. */
			return (0);
		}

		/*
		 * Calculate the size of the over-size run that arena_palloc()
		 * would need to allocate in order to guarantee the alignment.
		 * If the run wouldn't fit within a chunk, round up to a huge
		 * allocation size.
		 */
		run_size = usize + alignment - PAGE;
		if (run_size <= arena_maxclass)
			return (PAGE_CEILING(usize));
		return (CHUNK_CEILING(usize));
	}
}

JEMALLOC_INLINE unsigned
narenas_total_get(void)
{
	unsigned narenas;

	malloc_mutex_lock(&arenas_lock);
	narenas = narenas_total;
	malloc_mutex_unlock(&arenas_lock);

	return (narenas);
}

/* Choose an arena based on a per-thread value. */
JEMALLOC_INLINE arena_t *
choose_arena(arena_t *arena)
{
	arena_t *ret;

	if (arena != NULL)
		return (arena);

	if ((ret = *arenas_tsd_get()) == NULL) {
		ret = choose_arena_hard();
		assert(ret != NULL);
	}

	return (ret);
}
#endif

#include "jemalloc/internal/bitmap.h"
#include "jemalloc/internal/rtree.h"
/*
 * Include arena.h twice in order to resolve circular dependencies with
 * tcache.h.
 */
#define	JEMALLOC_ARENA_INLINE_A
#include "jemalloc/internal/arena.h"
#undef JEMALLOC_ARENA_INLINE_A
#include "jemalloc/internal/tcache.h"
#define	JEMALLOC_ARENA_INLINE_B
#include "jemalloc/internal/arena.h"
#undef JEMALLOC_ARENA_INLINE_B
#include "jemalloc/internal/hash.h"
#include "jemalloc/internal/quarantine.h"

#ifndef JEMALLOC_ENABLE_INLINE
void	*imallocx(size_t size, bool try_tcache, arena_t *arena);
void	*imalloc(size_t size);
void	*icallocx(size_t size, bool try_tcache, arena_t *arena);
void	*icalloc(size_t size);
void	*ipallocx(size_t usize, size_t alignment, bool zero, bool try_tcache,
    arena_t *arena);
void	*ipalloc(size_t usize, size_t alignment, bool zero);
size_t	isalloc(const void *ptr, bool demote);
size_t	ivsalloc(const void *ptr, bool demote);
size_t	u2rz(size_t usize);
size_t	p2rz(const void *ptr);
void	idallocx(void *ptr, bool try_tcache);
void	idalloc(void *ptr);
void	iqallocx(void *ptr, bool try_tcache);
void	iqalloc(void *ptr);
void	*irallocx(void *ptr, size_t size, size_t extra, size_t alignment,
    bool zero, bool no_move, bool try_tcache_alloc, bool try_tcache_dalloc,
    arena_t *arena);
void	*iralloc(void *ptr, size_t size, size_t extra, size_t alignment,
    bool zero, bool no_move);
malloc_tsd_protos(JEMALLOC_ATTR(unused), thread_allocated, thread_allocated_t)
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_C_))
JEMALLOC_ALWAYS_INLINE void *
imallocx(size_t size, bool try_tcache, arena_t *arena)
{

	assert(size != 0);

	if (size <= arena_maxclass)
		return (arena_malloc(arena, size, false, try_tcache));
	else
		return (huge_malloc(size, false));
}

JEMALLOC_ALWAYS_INLINE void *
imalloc(size_t size)
{

	return (imallocx(size, true, NULL));
}

JEMALLOC_ALWAYS_INLINE void *
icallocx(size_t size, bool try_tcache, arena_t *arena)
{

	if (size <= arena_maxclass)
		return (arena_malloc(arena, size, true, try_tcache));
	else
		return (huge_malloc(size, true));
}

JEMALLOC_ALWAYS_INLINE void *
icalloc(size_t size)
{

	return (icallocx(size, true, NULL));
}

JEMALLOC_ALWAYS_INLINE void *
ipallocx(size_t usize, size_t alignment, bool zero, bool try_tcache,
    arena_t *arena)
{
	void *ret;

	assert(usize != 0);
	assert(usize == sa2u(usize, alignment));

	if (usize <= arena_maxclass && alignment <= PAGE)
		ret = arena_malloc(arena, usize, zero, try_tcache);
	else {
		if (usize <= arena_maxclass) {
			ret = arena_palloc(choose_arena(arena), usize,
			    alignment, zero);
		} else if (alignment <= chunksize)
			ret = huge_malloc(usize, zero);
		else
			ret = huge_palloc(usize, alignment, zero);
	}

	assert(ALIGNMENT_ADDR2BASE(ret, alignment) == ret);
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void *
ipalloc(size_t usize, size_t alignment, bool zero)
{

	return (ipallocx(usize, alignment, zero, true, NULL));
}

/*
 * Typical usage:
 *   void *ptr = [...]
 *   size_t sz = isalloc(ptr, config_prof);
 */
JEMALLOC_ALWAYS_INLINE size_t
isalloc(const void *ptr, bool demote)
{
	size_t ret;
	arena_chunk_t *chunk;

	assert(ptr != NULL);
	/* Demotion only makes sense if config_prof is true. */
	assert(config_prof || demote == false);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr)
		ret = arena_salloc(ptr, demote);
	else
		ret = huge_salloc(ptr);

	return (ret);
}

JEMALLOC_ALWAYS_INLINE size_t
ivsalloc(const void *ptr, bool demote)
{

	/* Return 0 if ptr is not within a chunk managed by jemalloc. */
	if (rtree_get(chunks_rtree, (uintptr_t)CHUNK_ADDR2BASE(ptr)) == NULL)
		return (0);

	return (isalloc(ptr, demote));
}

JEMALLOC_INLINE size_t
u2rz(size_t usize)
{
	size_t ret;

	if (usize <= SMALL_MAXCLASS) {
		size_t binind = SMALL_SIZE2BIN(usize);
		ret = arena_bin_info[binind].redzone_size;
	} else
		ret = 0;

	return (ret);
}

JEMALLOC_INLINE size_t
p2rz(const void *ptr)
{
	size_t usize = isalloc(ptr, false);

	return (u2rz(usize));
}

JEMALLOC_ALWAYS_INLINE void
idallocx(void *ptr, bool try_tcache)
{
	arena_chunk_t *chunk;

	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr)
		arena_dalloc(chunk->arena, chunk, ptr, try_tcache);
	else
		huge_dalloc(ptr, true);
}

JEMALLOC_ALWAYS_INLINE void
idalloc(void *ptr)
{

	idallocx(ptr, true);
}

JEMALLOC_ALWAYS_INLINE void
iqallocx(void *ptr, bool try_tcache)
{

	if (config_fill && opt_quarantine)
		quarantine(ptr);
	else
		idallocx(ptr, try_tcache);
}

JEMALLOC_ALWAYS_INLINE void
iqalloc(void *ptr)
{

	iqallocx(ptr, true);
}

JEMALLOC_ALWAYS_INLINE void *
irallocx(void *ptr, size_t size, size_t extra, size_t alignment, bool zero,
    bool no_move, bool try_tcache_alloc, bool try_tcache_dalloc, arena_t *arena)
{
	void *ret;
	size_t oldsize;

	assert(ptr != NULL);
	assert(size != 0);

	oldsize = isalloc(ptr, config_prof);

	if (alignment != 0 && ((uintptr_t)ptr & ((uintptr_t)alignment-1))
	    != 0) {
		size_t usize, copysize;

		/*
		 * Existing object alignment is inadequate; allocate new space
		 * and copy.
		 */
		if (no_move)
			return (NULL);
		usize = sa2u(size + extra, alignment);
		if (usize == 0)
			return (NULL);
		ret = ipallocx(usize, alignment, zero, try_tcache_alloc, arena);
		if (ret == NULL) {
			if (extra == 0)
				return (NULL);
			/* Try again, without extra this time. */
			usize = sa2u(size, alignment);
			if (usize == 0)
				return (NULL);
			ret = ipallocx(usize, alignment, zero, try_tcache_alloc,
			    arena);
			if (ret == NULL)
				return (NULL);
		}
		/*
		 * Copy at most size bytes (not size+extra), since the caller
		 * has no expectation that the extra bytes will be reliably
		 * preserved.
		 */
		copysize = (size < oldsize) ? size : oldsize;
		memcpy(ret, ptr, copysize);
		iqallocx(ptr, try_tcache_dalloc);
		return (ret);
	}

	if (no_move) {
		if (size <= arena_maxclass) {
			return (arena_ralloc_no_move(ptr, oldsize, size,
			    extra, zero));
		} else {
			return (huge_ralloc_no_move(ptr, oldsize, size,
			    extra));
		}
	} else {
		if (size + extra <= arena_maxclass) {
			return (arena_ralloc(arena, ptr, oldsize, size, extra,
			    alignment, zero, try_tcache_alloc,
			    try_tcache_dalloc));
		} else {
			return (huge_ralloc(ptr, oldsize, size, extra,
			    alignment, zero, try_tcache_dalloc));
		}
	}
}

JEMALLOC_ALWAYS_INLINE void *
iralloc(void *ptr, size_t size, size_t extra, size_t alignment, bool zero,
    bool no_move)
{

	return (irallocx(ptr, size, extra, alignment, zero, no_move, true, true,
	    NULL));
}

malloc_tsd_externs(thread_allocated, thread_allocated_t)
malloc_tsd_funcs(JEMALLOC_ALWAYS_INLINE, thread_allocated, thread_allocated_t,
    THREAD_ALLOCATED_INITIALIZER, malloc_tsd_no_cleanup)
#endif

#include "jemalloc/internal/prof.h"

#undef JEMALLOC_H_INLINES
/******************************************************************************/
#endif /* JEMALLOC_INTERNAL_H */
