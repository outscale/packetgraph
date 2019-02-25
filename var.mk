PG_SOURCES = \
	src/queue.c\
	src/firewall.c\
	src/printer.c\
	src/packet.c\
	src/packets.c\
	src/lifecycle.c\
	src/nop.c\
	src/utils/mempool.c\
	src/utils/malloc.c\
	src/utils/bench.c\
	src/utils/qemu.c\
	src/utils/errors.c\
	src/utils/mac.c\
	src/utils/config.c\
	src/utils/tests.c\
	src/print.c\
	src/hub.c\
	src/brick.c\
	src/packetsgen.c\
	src/vhost.c\
	src/rxtx.c\
	src/user-dipole.c\
	src/npf/npf/dpdk/npf_dpdk.c\
	src/antispoof.c\
	src/collect.c\
	src/fail.c\
	src/vtep.c\
	src/vtep6.c\
	src/nic.c\
	src/tap.c\
	src/graph.c\
	src/diode.c\
	src/switch.c\
	src/pmtud.c\
	src/thread.c\
	src/ip-fragment.c
PG_OBJECTS = $(PG_SOURCES:.c=.o)
PG_HEADERS = \
	-I$(srcdir)/include/packetgraph/\
	-I$(srcdir)/src/npf/npf/src/libnpf/\
	-I$(srcdir)/src/npf/libqsbr/\
	-I$(srcdir)/src/npf/nvlist/src/\
	-I$(srcdir)/src/npf/thmap/src\
	-I$(srcdir)/src/npf/npf/src/kern/stand/\
	-I$(srcdir)/include\
	-I$(srcdir)/src\
	-I$(srcdir)\
	$(RTE_SDK_HEADERS)\
	$(GLIB_HEADERS)
PG_LIBADD = \
	$(RTE_SDK_LIBS)\
	$(GLIB_LIBS)
#FIXME '^pg_[^_]' does not take all symbols needed (i.e. __pg_error_*)
PG_LDFLAGS = \
	-version-info 17:5:0\
	-export-symbols-regex 'pg_[^_]'\
	-no-undefined
PG_CFLAGS = $(EXTRA_CFLAGS) -march=core-avx-i -mtune=core-avx-i -fmessage-length=0 -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11 $(GLIB_CFLAGS) $(RTE_SDK_CFLAGS) -Wno-implicite-fallthrough -Wno-deprecated-declarations -Wno-unknown-warning-option $(PG_ASAN_CFLAGS)

dist_doc_DATA = README.md

check_PROGRAMS = tests-antispoof tests-core tests-diode tests-rxtx tests-firewall tests-integration tests-nic tests-print tests-queue tests-switch tests-vhost tests-vtep  tests-pmtud tests-tap tests-ip-fragment tests-thread

PG_dev_SOURCES = $(PG_SOURCES)
PG_dev_OBJECTS = $(shell echo $(PG_dev_SOURCES:.c=.o) | sed -e "s/src/dev/g")
PG_dev_HEADERS = $(PG_HEADERS)
PG_dev_LIBADD = $(PG_LIBADD)
PG_dev_LDFLAGS = -no-undefined --export-all-symbols
PG_dev_CFLAGS = $(PG_CFLAGS) $(EXTRA_dev_CFLAGS) -D PG_NIC_STUB -D PG_NIC_BENCH -D PG_QUEUE_BENCH -D PG_VHOST_BENCH -D PG_RXTX_BENCH -D PG_TAP_BENCH -D PG_MALLOC_DEBUG

ACLOCAL_AMFLAGS = -I m4
