#pragma once

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#define __attribute_gcc_format(X) __attribute__ ((format X))
#define __attribute_nonnull __attribute__((nonnull))
#define __attribute_malloc __attribute__((malloc))
#define __attribute_warn_unused_result __attribute__((warn_unused_result))
#define __attribute_used __attribute__((used))
#define __attribute_unused __attribute__((unused))
#define __pack__ __attribute__((packed))
#ifdef WIN32
# pragma pack(1)
#endif
/*
 * The unaligned forms violate strict aliasing, which triggers a warning
 * in gcc 4.7 and above.
 */
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4)
#define COPY_WORDS_ALIGNED
#define DXX_HAVE_CXX11_EXPLICIT_DEFAULT
#define DXX_HAVE_CXX11_EXPLICIT_DELETE
#define DXX_HAVE_ISBLANK
#endif
#else
#define __noreturn
#define __attribute_gcc_format(X)
#define __attribute_nonnull
#define __attribute_malloc
#define __attribute_warn_unused_result
#define __attribute_used
#define __attribute_unused
#if defined(_MSC_VER)
# pragma pack(push, packing)
# pragma pack(1)
# define __pack__
#elif defined(macintosh)
# pragma options align=packed
# define __pack__
#else
# error d2x will not work without packed structures
#endif
#endif

#ifdef _MSC_VER
# define inline __inline
# define __func__ __FUNCTION__
# define __attribute_used__	/* */
#endif

#ifdef __cplusplus

#ifdef DXX_HAVE_CXX11_EXPLICIT_DEFAULT
#define DXX_CXX11_EXPLICIT_DEFAULT = default
#else
#define DXX_CXX11_EXPLICIT_DEFAULT
#endif

#ifdef DXX_HAVE_CXX11_EXPLICIT_DELETE
#define DXX_CXX11_EXPLICIT_DELETE = delete
#else
#define DXX_CXX11_EXPLICIT_DELETE
#endif

#if defined(DXX_HAVE_CXX11_EXPLICIT_DEFAULT) && \
	defined(DXX_HAVE_CXX11_EXPLICIT_DELETE)
/*
 * Strict checking requires C++11 features first supported in gcc 4.7.
 */
#define DXX_USE_STRICT_TYPESAFE
#endif

#if defined(__GNUC__) && defined(__OPTIMIZE__)
#ifdef __INLINE__
#define define_interposition_check(A,C,B)	\
	__errordecl(__trap_bad_argument_##A##_##B, #A #C #B);	\
	if (__builtin_constant_p(A C B) && A C B) __trap_bad_argument_##A##_##B()
#endif
#endif
#ifndef define_interposition_check
#define define_interposition_check(A,C,B)
#endif
#ifndef __extern_always_inline
#define __extern_always_inline static inline
#endif

#ifndef DECLSPEC_NORETURN
#define DECLSPEC_NORETURN
#endif

#define CHK_REDIRECT(R,N,A,B)	\
	R (unchecked_##N) A;	\
	__extern_always_inline R (N) A;	\
	__extern_always_inline R (N) A	\
	{ B; }

#endif
