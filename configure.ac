AC_PREREQ([2.69])
AC_INIT([packetgraph], [17.05.0], [jerome.jutteau@outscale.com matthias.gatto@outscale.com], [], [https://github.com/outscale/packetgraph])
AM_INIT_AUTOMAKE([foreign -Wall subdir-objects])
#AC_CONFIG_SRCDIR([include/packetgraph/print.h])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_CHECK_PROG(have_pkg_config, [pkg-config], [yes], [no])

# Options & parameters
AC_MSG_CHECKING([packetgraph examples])
AC_ARG_WITH(examples, AS_HELP_STRING([--with-examples], [Build examples]))
AM_CONDITIONAL([PG_EXAMPLES], [test "x$with_examples" = "xyes"])
AS_IF([test "x$with_examples" = "xyes"], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([packetgraph benchmarks])
AC_ARG_WITH(benchmarks, AS_HELP_STRING([--with-benchmarks], [Build benchmarks]))
AM_CONDITIONAL([PG_BENCHMARKS], [test "x$with_benchmarks" = "xyes"])
AS_IF([test "x$with_benchmarks" = "xyes"], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([packetgraph build npf])
AC_ARG_WITH(build-npf, AS_HELP_STRING([--without-build-npf], [Avoid building npf and sub-libraries]))
AM_CONDITIONAL([PG_BUILD_NPF], [test "x$with_build_npf" != "xno"])
AS_IF([test "x$with_build_npf" = "xno"], [AC_MSG_RESULT([no])], [AC_MSG_RESULT([yes])])

AC_MSG_CHECKING([RTE_SDK parameter])
AC_ARG_VAR([RTE_SDK], [Path to DPDK reprository])
# FIXME: hope to have DPDK correctly packaged one day.
AS_IF([test "x$RTE_SDK" != "x"], [
    AC_MSG_RESULT([$RTE_SDK])
    AC_SUBST(RTE_SDK_CFLAGS, ["-I$RTE_SDK/build/include -D RTE_MAX_LCORE=128 -D RTE_PKTMBUF_HEADROOM=128"])
    AC_SUBST(RTE_SDK_HEADERS_FOLDER, [$RTE_SDK/build/include/])
    AC_SUBST(RTE_SDK_LIBS, [" -lm -lpthread -ldl -lpcap -L$RTE_SDK/build/lib/ -ldpdk "])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Did you set RTE_SDK variable ?])
])

# Checks for libraries
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.34 gobject-2.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SEARCH_LIBS([pthread_attr_init], [pthread], [], [AC_MSG_ERROR([Unable to find pthread library])])
AC_SEARCH_LIBS([pcap_open_dead], [pcap], [], [AC_MSG_ERROR([Unable to find pcap library])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_pcap.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_pcap.a])])
AS_IF([test -f $RTE_SDK/build/lib/libdpdk.a], [], [AC_MSG_ERROR([Unable to find libdpdk.a])])
AS_IF([test "x$with_build_npf" = "xno"], [
        AC_SEARCH_LIBS([npf_config_create], [npf], [], [AC_MSG_ERROR([Unable to find npf library])], [-lm -lnpfkern -lpthread -ldl])
        AC_SEARCH_LIBS([npf_create], [npfkern], [], [AC_MSG_ERROR([Unable to find npfkern library])], [-lm -ldl])
	AC_CHECK_HEADERS([npf.h], [], [AC_MSG_ERROR([Unable to find npf headers])])
        ], [
        # Problib stuff
        AC_CHECK_HEADERS(limits.h stddef.h fcntl.h)
        AC_CHECK_FUNCS(memmove memset munmap strchr strtoul stroull strlcat fdatasync)
        AC_CHECK_DECLS(fdatasync, [], [], ['#include <unistd.h>'])
        AC_CHECK_LIB(pthread, pthread_mutex_init, ,
            AC_MSG_ERROR(*** A POSIX threads library is required, aborting ***))
        AC_CHECK_LIB(z, inflate, , AC_MSG_ERROR(*** zlib is required, aborting ***))

        AC_MSG_CHECKING(for GCC atomic builtins)
        AC_COMPILE_IFELSE(
            [AC_LANG_SOURCE(
                [[#include <stdint.h>
                int main() {
                volatile unsigned long val = 1;
                __sync_fetch_and_add(&val, 1);
                __sync_fetch_and_sub(&val, 1);
                __sync_add_and_fetch(&val, 1);
                __sync_sub_and_fetch(&val, 1);
                return 0;
                }
                ]])],
            [AC_MSG_RESULT(yes); AC_DEFINE([HAVE_ATOMICS], [1], [GCC atomics support])],
            [AC_MSG_RESULT(no)])

        AC_C_CONST
        AC_FUNC_MEMCMP
        AC_FUNC_MMAP
        AC_HEADER_STDC
        AC_TYPE_INT16_T
        AC_TYPE_INT8_T
        AC_TYPE_MODE_T
        AC_TYPE_OFF_T
])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h limits.h stdint.h stdlib.h string.h sys/time.h unistd.h], [], [AC_MSG_ERROR([Unable to find some basic headers])])
AC_CHECK_HEADERS([${RTE_SDK_HEADERS_FOLDER}rte_config.h], [], [AC_MSG_ERROR([Unable to find dpdk headers])])
AC_CHECK_HEADERS(jemalloc/jemalloc.h, [], [Unable to find jemalloc])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([gettimeofday memset realpath])

AC_CONFIG_FILES([Makefile])

# libtoolize recommanded this
AC_CONFIG_MACRO_DIR([m4])

# To avoid messages from automake
# usr/share/automake-1.13/am/ltlibrary.am: warning: 'libpacketgraph.la': linking libtool libraries using a non-POSIX
# /usr/share/automake-1.13/am/ltlibrary.am: archiver requires 'AM_PROG_AR' in 'configure.ac'
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
AC_OUTPUT
