/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* big endian */
/* #undef ENDIAN_BIG */

#ifndef CONFIGURED_MASSTREE
#define CONFIGURED_MASSTREE

/* little endian */
#define ENDIAN_LITTLE 1

/* Define to enable debugging assertions. */
#define HAVE_ASSERTIONS_ENABLED 1

/* Define to 1 if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define if the C++ compiler understands 'auto'. */
#define HAVE_CXX_AUTO 0

/* Define if the C++ compiler understands constexpr. */
#define HAVE_CXX_CONSTEXPR 0

/* Define if the C++ compiler understands rvalue references. */
#define HAVE_CXX_RVALUE_REFERENCES 0

/* Define if the C++ compiler understands static_assert. */
#define HAVE_CXX_STATIC_ASSERT 1

/* Define if the C++ compiler understands template alias. */
#define HAVE_CXX_TEMPLATE_ALIAS 1

/* Define to 1 if you have the declaration of `clock_gettime', and to 0 if you
   don't. */
#define HAVE_DECL_CLOCK_GETTIME 0

/* Define to 1 if you have the declaration of `getline', and to 0 if you
   don't. */
#define HAVE_DECL_GETLINE 1

/* Define if you are using libflow for malloc. */
/* #undef HAVE_FLOW_MALLOC */

/* Define if you are using libhoard for malloc. */
/* #undef HAVE_HOARD_MALLOC */

/* Define if int64_t and long are the same type. */
/* #undef HAVE_INT64_T_IS_LONG */

/* Define if int64_t and long long are the same type. */
#define HAVE_INT64_T_IS_LONG_LONG 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to enable invariant assertions. */
/* #undef HAVE_INVARIANTS_ENABLED */

/* Define if you are using libjemalloc for malloc. */
/* #undef HAVE_JEMALLOC */

/* Define if you have libnuma. */
/* #undef HAVE_LIBNUMA */

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define if memory debugging support is enabled. */
/* #undef HAVE_MEMDEBUG */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <numa.h> header file. */
/* #undef HAVE_NUMA_H */

/* Define if off_t and long are the same type. */
/* #undef HAVE_OFF_T_IS_LONG */

/* Define if off_t and long long are the same type. */
#define HAVE_OFF_T_IS_LONG_LONG 1

/* Define to enable precondition assertions. */
/* #undef HAVE_PRECONDITIONS_ENABLED */

/* Define if size_t and unsigned are the same type. */
/* #undef HAVE_SIZE_T_IS_UNSIGNED */

/* Define if size_t and unsigned long are the same type. */
#define HAVE_SIZE_T_IS_UNSIGNED_LONG 1

/* Define if size_t and unsigned long long are the same type. */
/* #undef HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define if you have the std::is_rvalue_reference template. */
#define HAVE_STD_IS_RVALUE_REFERENCE 1

/* Define if you have the std::is_trivially_copyable template. */
#define HAVE_STD_IS_TRIVIALLY_COPYABLE 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if superpage support is enabled. */
#define HAVE_SUPERPAGE_ENABLED 1

/* Define to 1 if you have the <sys/epoll.h> header file. */
/* #undef HAVE_SYS_EPOLL_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you are using libtcmalloc for malloc. */
/* #undef HAVE_TCMALLOC */

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <type_traits> header file. */
#define HAVE_TYPE_TRAITS 0

/* Define if unaligned accesses are OK. */
#define HAVE_UNALIGNED_ACCESS 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you have the __builtin_clz builtin. */
#define HAVE___BUILTIN_CLZ 1

/* Define if you have the __builtin_clzl builtin. */
#define HAVE___BUILTIN_CLZL 1

/* Define if you have the __builtin_clzll builtin. */
#define HAVE___BUILTIN_CLZLL 1

/* Define if you have the __builtin_ctz builtin. */
#define HAVE___BUILTIN_CTZ 1

/* Define if you have the __builtin_ctzl builtin. */
#define HAVE___BUILTIN_CTZL 1

/* Define if you have the __builtin_ctzll builtin. */
#define HAVE___BUILTIN_CTZLL 1

/* Define if you have the __has_trivial_copy compiler intrinsic. */
#define HAVE___HAS_TRIVIAL_COPY 1

/* Define if you have the __sync_add_and_fetch builtin. */
#define HAVE___SYNC_ADD_AND_FETCH 1

/* Define if you have the __sync_add_and_fetch_8 builtin. */
#define HAVE___SYNC_ADD_AND_FETCH_8 1

/* Define if you have the __sync_bool_compare_and_swap builtin. */
#define HAVE___SYNC_BOOL_COMPARE_AND_SWAP 1

/* Define if you have the __sync_bool_compare_and_swap_8 builtin. */
#define HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 1

/* Define if you have the __sync_fetch_and_add builtin. */
#define HAVE___SYNC_FETCH_AND_ADD 1

/* Define if you have the __sync_fetch_and_add_8 builtin. */
#define HAVE___SYNC_FETCH_AND_ADD_8 1

/* Define if you have the __sync_fetch_and_or builtin. */
#define HAVE___SYNC_FETCH_AND_OR 1

/* Define if you have the __sync_fetch_and_or_8 builtin. */
#define HAVE___SYNC_FETCH_AND_OR_8 1

/* Define if you have the __sync_lock_release_set builtin. */
#define HAVE___SYNC_LOCK_RELEASE_SET 1

/* Define if you have the __sync_lock_test_and_set builtin. */
#define HAVE___SYNC_LOCK_TEST_AND_SET 1

/* Define if you have the __sync_lock_test_and_set_val builtin. */
#define HAVE___SYNC_LOCK_TEST_AND_SET_VAL 1

/* Define if you have the __sync_or_and_fetch builtin. */
#define HAVE___SYNC_OR_AND_FETCH 1

/* Define if you have the __sync_or_and_fetch_8 builtin. */
#define HAVE___SYNC_OR_AND_FETCH_8 1

/* Define if you have the __sync_synchronize builtin. */
#define HAVE___SYNC_SYNCHRONIZE 1

/* Define if you have the __sync_val_compare_and_swap builtin. */
#define HAVE___SYNC_VAL_COMPARE_AND_SWAP 1

/* Define if you have the __sync_val_compare_and_swap_8 builtin. */
#define HAVE___SYNC_VAL_COMPARE_AND_SWAP_8 1

/* Maximum key length */
#define KVDB_MAX_KEY_LEN 255

/* Maximum row length */
#define KVDB_MAX_ROW_LEN 65535

/* Define if the default row type is kvdb_timed_array. */
#define KVDB_ROW_TYPE_ARRAY 1

/* Define if the default row type is kvdb_timed_array_ver. */
/* #undef KVDB_ROW_TYPE_ARRAY_VER */

/* Define if the default row type is kvdb_timed_bag. */
//#define KVDB_ROW_TYPE_BAG 1

/* Define if the default row type is kvdb_timed_str. */
//#define KVDB_ROW_TYPE_STR 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "masstree-beta"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "masstree-beta 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "masstree-beta"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 8

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

#if !HAVE_ALL_ASSERTIONS_ENABLED && !HAVE_ASSERTIONS_ENABLED
# define NDEBUG 1
#endif

/** @brief Assert macro that always runs. */
extern void fail_mandatory_assert(const char* file, int line, const char* assertion, const char* message = 0) __attribute__((noreturn));
#define mandatory_assert(x, ...) do { if (!(x)) fail_mandatory_assert(__FILE__, __LINE__, #x, ## __VA_ARGS__); } while (0)

/** @brief Assert macro for invariants.

    invariant(x) is executed if --enable-invariants or --enable-assertions. */
extern void fail_invariant(const char* file, int line, const char* assertion, const char* message = 0) __attribute__((noreturn));
#if HAVE_ALL_ASSERTIONS_ENABLED || (!defined(HAVE_INVARIANTS_ENABLED) && HAVE_ASSERTIONS_ENABLED) || HAVE_INVARIANTS_ENABLED
#define invariant(x, ...) do { if (!(x)) fail_invariant(__FILE__, __LINE__, #x, ## __VA_ARGS__); } while (0)
#else
#define invariant(x, ...) do { } while (0)
#endif

/** @brief Assert macro for preconditions.

    precondition(x) is executed if --enable-preconditions or
    --enable-assertions. */
extern void fail_precondition(const char* file, int line, const char* assertion, const char* message = 0) __attribute__((noreturn));
#if HAVE_ALL_ASSERTIONS_ENABLED || (!defined(HAVE_PRECONDITIONS_ENABLED) && HAVE_ASSERTIONS_ENABLED) || HAVE_PRECONDITIONS_ENABLED
#define precondition(x, ...) do { if (!(x)) fail_precondition(__FILE__, __LINE__, #x, ## __VA_ARGS__); } while (0)
#else
#define precondition(x, ...) do { } while (0)
#endif


#endif //CONFIGURED_MASSTREE
