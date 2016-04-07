lib_LTLIBRARIES = libpacketgraph.la

# No wildcard
# https://www.gnu.org/software/automake/manual/html_node/Wildcards.html
libpacketgraph_la_SOURCES = \
	src/queue.c\
	src/firewall.c\
	src/printer.c\
	src/packets.c\
	src/lifecycle.c\
	src/nop.c\
	src/utils/mempool.c\
	src/utils/bench.c\
	src/utils/qemu.c\
	src/utils/errors.c\
	src/utils/mac.c\
	src/utils/config.c\
	src/utils/bitmask.c\
	src/print.c\
	src/hub.c\
	src/brick.c\
	src/packetsgen.c\
	src/vhost.c\
	src/npf_dpdk.c\
	src/antispoof.c\
	src/collect.c\
	src/vtep.c\
	src/nic.c\
	src/graph.c\
	src/diode.c\
	src/switch.c

libpacketgraph_la_LIBADD = $(RTE_SDK_LIBS) $(GLIB_LIBS) -lnpf -lnpfkern
# FIXME '^pg_[^_]' does not take all symbols needed (i.e. __pg_error_*)
libpacketgraph_la_LDFLAGS = -version-info 0:0:0 -export-symbols-regex 'pg_[^_]' -no-undefined $(RTE_SDK_LDFLAGS)
libpacketgraph_la_CFLAGS = -Ofast -march=core-avx-i -mtune=core-avx-i -fmessage-length=0 -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11 $(GLIB_CFLAGS) $(RTE_SDK_CFLAGS) -I$(srcdir)/include -I$(srcdir)/src

pkginclude_HEADERS = \
	include/packetgraph/common.h\
	include/packetgraph/print.h\
	include/packetgraph/hub.h\
	include/packetgraph/packetgraph.h\
	include/packetgraph/diode.h\
	include/packetgraph/nop.h\
	include/packetgraph/nic.h\
	include/packetgraph/antispoof.h\
	include/packetgraph/brick.h\
	include/packetgraph/lifecycle.h\
	include/packetgraph/graph.h\
	include/packetgraph/vtep.h\
	include/packetgraph/firewall.h\
	include/packetgraph/vhost.h\
	include/packetgraph/switch.h\
	include/packetgraph/queue.h\
	include/packetgraph/errors.h

dist_doc_DATA = README.md

check_PROGRAMS = tests-antispoof tests-core tests-diode tests-firewall tests-integration tests-nic tests-print tests-switch tests-vhost tests-vtep

noinst_LTLIBRARIES = libpacketgraph-dev.la
libpacketgraph_dev_la_SOURCES = $(libpacketgraph_la_SOURCES)
libpacketgraph_dev_la_LIBADD = $(libpacketgraph_la_LIBADD)
libpacketgraph_dev_la_LDFLAGS = -no-undefined --export-all-symbols $(RTE_SDK_LDFLAGS)
libpacketgraph_dev_la_CFLAGS = -g $(libpacketgraph_la_CFLAGS) -D PG_NIC_STUB -D PG_NIC_BENCH -D PG_VHOST_BENCH

tests_antispoof_SOURCES = \
	tests/antispoof/test-arp-gratuitous.c\
	tests/antispoof/test-arp-response.c\
	tests/antispoof/test-rarp.c\
	tests/antispoof/tests.c
tests_antispoof_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_antispoof_LDADD = libpacketgraph-dev.la

tests_core_SOURCES = \
	tests/core/test-core.c\
	tests/core/test-graph.c\
	tests/core/test-flow.c\
	tests/core/test-error.c\
	tests/core/test-pkts-count.c\
	tests/core/test-hub.c\
	tests/core/tests.c
tests_core_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_core_LDFLAGS = libpacketgraph-dev.la

tests_diode_SOURCES = \
	tests/diode/tests.c
tests_diode_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_diode_LDFLAGS = libpacketgraph-dev.la

tests_firewall_SOURCES = \
	tests/firewall/test-icmp.c\
	tests/firewall/tests.c\
	tests/firewall/test-tcp.c
tests_firewall_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_firewall_LDFLAGS = libpacketgraph-dev.la

tests_integration_SOURCES = \
	tests/integration/tests.c
tests_integration_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_integration_LDFLAGS = libpacketgraph-dev.la

tests_nic_SOURCES = \
	tests/nic/test-nic.c\
	tests/nic/tests.c
tests_nic_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_nic_LDFLAGS = libpacketgraph-dev.la

tests_print_SOURCES = \
	tests/print/tests.c
tests_print_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_print_LDFLAGS = libpacketgraph-dev.la

tests_switch_SOURCES = \
	tests/switch/tests.c
tests_switch_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_switch_LDFLAGS = libpacketgraph-dev.la

tests_vhost_SOURCES = \
	tests/vhost/tests.c\
	tests/vhost/test-vhost.c
tests_vhost_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_vhost_LDFLAGS = libpacketgraph-dev.la

tests_vtep_SOURCES = \
	tests/vtep/tests.c
tests_vtep_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_vtep_LDFLAGS = libpacketgraph-dev.la

TESTS = \
	tests/antispoof/test.sh\
	tests/core/test.sh\
	tests/diode/test.sh\
	tests/firewall/test.sh\
	tests/integration/test.sh\
	tests/nic/test.sh\
	tests/print/test.sh\
	tests/switch/test.sh\
	tests/vhost/test.sh\
	tests/vtep/test.sh

if PG_BENCHMARKS
noinst_PROGRAMS = bench-antispoof bench-core bench-diode bench-firewall bench-nic bench-print bench-switch bench-vhost bench-vtep

bench_antispoof_SOURCES = \
	tests/antispoof/bench.c\
	tests/antispoof/bench-antispoof.c
bench_antispoof_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_antispoof_LDFLAGS = libpacketgraph-dev.la

bench_core_SOURCES = \
	tests/core/bench-hub.c\
	tests/core/bench-nop.c\
	tests/core/bench.c
bench_core_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_core_LDFLAGS = libpacketgraph-dev.la

bench_diode_SOURCES = \
	tests/diode/bench.c\
	tests/diode/bench-diode.c
bench_diode_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_diode_LDFLAGS = libpacketgraph-dev.la

bench_firewall_SOURCES = \
	tests/firewall/bench.c\
	tests/firewall/bench-firewall.c
bench_firewall_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_firewall_LDFLAGS = libpacketgraph-dev.la

bench_nic_SOURCES = \
	tests/nic/bench.c\
	tests/nic/bench-nic.c
bench_nic_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_nic_LDFLAGS = libpacketgraph-dev.la

bench_print_SOURCES = \
	tests/print/bench-print.c\
	tests/print/bench.c
bench_print_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_print_LDFLAGS = libpacketgraph-dev.la

bench_switch_SOURCES = \
	tests/switch/bench-switch.c\
	tests/switch/bench.c
bench_switch_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_switch_LDFLAGS = libpacketgraph-dev.la

bench_vhost_SOURCES = \
	tests/vhost/bench-vhost.c\
	tests/vhost/bench.c
bench_vhost_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_vhost_LDFLAGS = libpacketgraph-dev.la

bench_vtep_SOURCES = \
	tests/vtep/bench-vtep.c\
	tests/vtep/bench.c
bench_vtep_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_vtep_LDFLAGS = libpacketgraph-dev.la

bench: bench-antispoof bench-core bench-diode bench-firewall bench-nic bench-print bench-switch bench-vhost bench-vtep
	$(srcdir)/tests/antispoof/bench.sh
	$(srcdir)/tests/core/bench.sh
	$(srcdir)/tests/diode/bench.sh
	$(srcdir)/tests/firewall/bench.sh
	$(srcdir)/tests/nic/bench.sh
	$(srcdir)/tests/print/bench.sh
	$(srcdir)/tests/switch/bench.sh
	$(srcdir)/tests/vtep/bench.sh
endif

style:
	$(srcdir)/tests/style/test.sh $(srcdir)

help:
	@echo "make         : build packetgraph"
	@echo "make doc     : generate public documentation using doxygen"
	@echo "make style   : run style tests"
	@echo "make check   : run tests"
	@echo "make install : install packetgraph library"
if PG_BENCHMARKS
	@echo "make bench   : run benchmarks"
endif
	@echo ""
	@echo "if you need to run a specific test, i.e. vtep:"
	@echo "./tests/vtep/test.sh"

if PG_EXAMPLES
# TODO
endif

doxygen.conf: $(srcdir)/doc/doxygen.conf.template
	$(SED) "s|PG_SRC_PATH|@srcdir@|g" $< > $@

.PHONY: doc
doc: doxygen.conf
	doxygen $^

# libtoolize recommanded this
ACLOCAL_AMFLAGS = -I m4
