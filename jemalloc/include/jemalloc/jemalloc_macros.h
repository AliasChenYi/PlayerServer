#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <strings.h>

#define	JEMALLOC_VERSION "jemalloc-cmake.4.3.1-1-gabeb19f8493feb67a2a8f121ea9bab1829f8d3fb"
#define	JEMALLOC_VERSION_MAJOR jemalloc
#define	JEMALLOC_VERSION_MINOR cmake
#define	JEMALLOC_VERSION_BUGFIX 4
#define	JEMALLOC_VERSION_NREV 3
#define	JEMALLOC_VERSION_GID "1"

#  define MALLOCX_LG_ALIGN(la)	((int)(la))
#  if LG_SIZEOF_PTR == 2
#    define MALLOCX_ALIGN(a)	((int)(ffs((int)(a))-1))
#  else
#    define MALLOCX_ALIGN(a)						\
       ((int)(((size_t)(a) < (size_t)INT_MAX) ? ffs((int)(a))-1 :	\
       ffs((int)(((size_t)(a))>>32))+31))
#  endif
#  define MALLOCX_ZERO	((int)0x40)
/*
 * Bias tcache index bits so that 0 encodes "automatic tcache management", and 1
 * encodes MALLOCX_TCACHE_NONE.
 */
#  define MALLOCX_TCACHE(tc)	((int)(((tc)+2) << 8))
#  define MALLOCX_TCACHE_NONE	MALLOCX_TCACHE(-1)
/*
 * Bias arena index bits so that 0 encodes "use an automatically chosen arena".
 */
#  define MALLOCX_ARENA(a)	((((int)(a))+1) << 20)

#if defined(__cplusplus) && defined(JEMALLOC_USE_CXX_THROW)
#  define JEMALLOC_CXX_THROW throw()
#else
#  define JEMALLOC_CXX_THROW
#endif

#if _MSC_VER
#  define JEMALLOC_ATTR(s)
#  define JEMALLOC_ALIGNED(s) __declspec(align(s))
#  define JEMALLOC_ALLOC_SIZE(s)
#  define JEMALLOC_ALLOC_SIZE2(s1, s2)
#  ifndef JEMALLOC_EXPORT
#    ifdef DLLEXPORT
#      define JEMALLOC_EXPORT __declspec(dllexport)
#    else
#      define JEMALLOC_EXPORT __declspec(dllimport)
#    endif
#  endif
#  define JEMALLOC_FORMAT_PRINTF(s, i)
#  define JEMALLOC_NOINLINE __declspec(noinline)
#  ifdef __cplusplus
#    define JEMALLOC_NOTHROW __declspec(nothrow)
#  else
#    define JEMALLOC_NOTHROW
#  endif
#  define JEMALLOC_SECTION(s) __declspec(allocate(s))
#  define JEMALLOC_RESTRICT_RETURN __declspec(restrict)
#  if _MSC_VER >= 1900 && !defined(__EDG__)
#    define JEMALLOC_ALLOCATOR __declspec(allocator)
#  else
#    define JEMALLOC_ALLOCATOR
#  endif
#elif defined(JEMALLOC_HAVE_ATTR)
#  define JEMALLOC_ATTR(s) __attribute__((s))
#  define JEMALLOC_ALIGNED(s) JEMALLOC_ATTR(aligned(s))
#  ifdef JEMALLOC_HAVE_ATTR_ALLOC_SIZE
#    define JEMALLOC_ALLOC_SIZE(s) JEMALLOC_ATTR(alloc_size(s))
#    define JEMALLOC_ALLOC_SIZE2(s1, s2) JEMALLOC_ATTR(alloc_size(s1, s2))
#  else
#    define JEMALLOC_ALLOC_SIZE(s)
#    define JEMALLOC_ALLOC_SIZE2(s1, s2)
#  endif
#  ifndef JEMALLOC_EXPORT
#    define JEMALLOC_EXPORT JEMALLOC_ATTR(visibility("default"))
#  endif
#  ifdef JEMALLOC_HAVE_ATTR_FORMAT_GNU_PRINTF
#    define JEMALLOC_FORMAT_PRINTF(s, i) JEMALLOC_ATTR(format(gnu_printf, s, i))
#  elif defined(JEMALLOC_HAVE_ATTR_FORMAT_PRINTF)
#    define JEMALLOC_FORMAT_PRINTF(s, i) JEMALLOC_ATTR(format(printf, s, i))
#  else
#    define JEMALLOC_FORMAT_PRINTF(s, i)
#  endif
#  define JEMALLOC_NOINLINE JEMALLOC_ATTR(noinline)
#  define JEMALLOC_NOTHROW JEMALLOC_ATTR(nothrow)
#  define JEMALLOC_SECTION(s) JEMALLOC_ATTR(section(s))
#  define JEMALLOC_RESTRICT_RETURN
#  define JEMALLOC_ALLOCATOR
#else
#  define JEMALLOC_ATTR(s)
#  define JEMALLOC_ALIGNED(s)
#  define JEMALLOC_ALLOC_SIZE(s)
#  define JEMALLOC_ALLOC_SIZE2(s1, s2)
#  define JEMALLOC_EXPORT
#  define JEMALLOC_FORMAT_PRINTF(s, i)
#  define JEMALLOC_NOINLINE
#  define JEMALLOC_NOTHROW
#  define JEMALLOC_SECTION(s)
#  define JEMALLOC_RESTRICT_RETURN
#  define JEMALLOC_ALLOCATOR
#endif
