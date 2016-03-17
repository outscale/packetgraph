AC_PREREQ([2.69])
AC_INIT([packetgraph], [0.0.1], [jerome.jutteau@outscale.com matthias.gatto@outscale.com], [], [https://github.com/outscale/packetgraph])
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

AC_MSG_CHECKING([RTE_SDK parameter])
AC_ARG_VAR([RTE_SDK], [Path to DPDK reprository])
# FIXME: hope to have DPDK correctly packaged one day.
AS_IF([test "x$RTE_SDK" != "x"], [
    AC_MSG_RESULT([$RTE_SDK])
    AC_SUBST(RTE_SDK_CFLAGS, ["-I$RTE_SDK/build/include -D RTE_MAX_LCORE=128 -D RTE_PKTMBUF_HEADROOM=128"])
    AC_SUBST(RTE_SDK_HEADERS_FOLDER, [$RTE_SDK/build/include/])
    AC_SUBST(RTE_SDK_LDFLAGS, ["-L$RTE_SDK/build/lib/ -lrte_pmd_vhost -lrte_hash -lrte_mbuf -lrte_mempool -lrte_table -lrte_ring -lethdev -lrte_pmd_pcap -lrte_pmd_ring -lrte_pmd_e1000 -lrte_pmd_ixgbe -lrte_kvargs -lrte_eal -lrte_vhost"])
    AC_SUBST(RTE_SDK_LIBS, ["-lm -lpthread -ldl -lpcap"])
], [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Did you set RTE_SDK variable ?])
])

# Checks for libraries
PKG_CHECK_MODULES(GLIB, glib-2.0 gobject-2.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SEARCH_LIBS([pthread_attr_init], [pthread], [], [AC_MSG_ERROR([Unable to find pthread library])])
AC_SEARCH_LIBS([npf_config_create], [npf], [], [AC_MSG_ERROR([Unable to find npf library])], [-lm -lnpfkern -lpthread -ldl])
AC_SEARCH_LIBS([npf_create], [npfkern], [], [AC_MSG_ERROR([Unable to find npfkern library])], [-lm -ldl])
AS_IF([test -f $RTE_SDK/build/lib/librte_vhost.a], [], [AC_MSG_ERROR([Unable to find librte_vhost.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_vhost.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_vhost.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_hash.a], [], [AC_MSG_ERROR([Unable to find librte_hash.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_mbuf.a], [], [AC_MSG_ERROR([Unable to find librte_mbuf.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_mempool.a], [], [AC_MSG_ERROR([Unable to find librte_mempool.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_table.a], [], [AC_MSG_ERROR([Unable to find librte_table.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_ring.a], [], [AC_MSG_ERROR([Unable to find librte_ring.a])])
AS_IF([test -f $RTE_SDK/build/lib/libethdev.a], [], [AC_MSG_ERROR([Unable to find libethdev.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_pcap.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_pcap.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_ring.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_ring.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_e1000.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_e1000.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_pmd_ixgbe.a], [], [AC_MSG_ERROR([Unable to find librte_pmd_ixgbe.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_kvargs.a], [], [AC_MSG_ERROR([Unable to find librte_kvargs.a])])
AS_IF([test -f $RTE_SDK/build/lib/librte_eal.a], [], [AC_MSG_ERROR([Unable to find librte_eal.a])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h limits.h stdint.h stdlib.h string.h sys/time.h unistd.h], [], [AC_MSG_ERROR([Unable to find some basic headers])])
AC_CHECK_HEADERS([npf.h], [], [AC_MSG_ERROR([Unable to find npf headers])])
AC_CHECK_HEADERS([${RTE_SDK_HEADERS_FOLDER}rte_config.h], [], [AC_MSG_ERROR([Unable to find dpdk headers])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
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
