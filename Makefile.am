lib_LTLIBRARIES = libpacketgraph.la

if PG_ASAN
PG_ASAN_CFLAGS= -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-sanitize=alignment --param asan-globals=0
endif

# No wildcard
# https://www.gnu.org/software/automake/manual/html_node/Wildcards.html
libpacketgraph_la_SOURCES = \
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

pkginclude_HEADERS = \
	include/packetgraph/common.h\
	include/packetgraph/print.h\
	include/packetgraph/hub.h\
	include/packetgraph/packetgraph.h\
	include/packetgraph/packet.h\
	include/packetgraph/diode.h\
	include/packetgraph/nop.h\
	include/packetgraph/nic.h\
	include/packetgraph/tap.h\
	include/packetgraph/antispoof.h\
	include/packetgraph/brick.h\
	include/packetgraph/lifecycle.h\
	include/packetgraph/graph.h\
	include/packetgraph/thread.h\
	include/packetgraph/vtep.h\
	include/packetgraph/firewall.h\
	include/packetgraph/vhost.h\
	include/packetgraph/rxtx.h\
	include/packetgraph/user-dipole.h\
	include/packetgraph/switch.h\
	include/packetgraph/queue.h\
	include/packetgraph/pmtud.h\
	include/packetgraph/ip-fragment.h\
	include/packetgraph/errors.h

libpacketgraph_la_LIBADD = $(RTE_SDK_LIBS) $(GLIB_LIBS)
# FIXME '^pg_[^_]' does not take all symbols needed (i.e. __pg_error_*)
libpacketgraph_la_LDFLAGS = -version-info 17:5:0 -export-symbols-regex 'pg_[^_]' -no-undefined
libpacketgraph_la_CFLAGS = -march=core-avx-i -mtune=core-avx-i -fmessage-length=0 -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11 $(GLIB_CFLAGS) $(RTE_SDK_CFLAGS) -I$(srcdir)/include -I$(srcdir)/src -Wno-implicite-fallthrough -Wno-deprecated-declarations -Wno-unknown-warning-option $(PG_ASAN_CFLAGS)

noinst_LTLIBRARIES =

include npfmakefile.am

dist_doc_DATA = README.md

check_PROGRAMS = tests-antispoof tests-core tests-diode tests-rxtx tests-firewall tests-integration tests-nic tests-print tests-queue tests-switch tests-vhost tests-vtep  tests-pmtud tests-tap tests-ip-fragment tests-thread

noinst_LTLIBRARIES += libpacketgraph-dev.la
libpacketgraph_dev_la_SOURCES = $(libpacketgraph_la_SOURCES)
libpacketgraph_dev_la_LIBADD = $(libpacketgraph_la_LIBADD)
libpacketgraph_dev_la_LDFLAGS = -no-undefined --export-all-symbols
libpacketgraph_dev_la_CFLAGS = $(libpacketgraph_la_CFLAGS) -D PG_NIC_STUB -D PG_NIC_BENCH -D PG_QUEUE_BENCH -D PG_VHOST_BENCH -D PG_RXTX_BENCH -D PG_TAP_BENCH -D PG_MALLOC_DEBUG

tests_antispoof_SOURCES = \
	tests/antispoof/test-arp-gratuitous.c\
	tests/antispoof/test-arp-response.c\
	tests/antispoof/test-rarp.c\
	tests/antispoof/tests.c
tests_antispoof_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_antispoof_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_antispoof_DEPENDENCIES = libpacketgraph-dev.la

tests_core_SOURCES = \
	tests/core/test-bitmask.c\
	tests/core/test-core.c\
	tests/core/test-dot.c\
	tests/core/test-flow.c\
	tests/core/test-error.c\
	tests/core/test-pkts-count.c\
	tests/core/test-graph.c\
	tests/core/test-hub.c\
	tests/core/tests.c
tests_core_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_core_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_core_DEPENDENCIES = libpacketgraph-dev.la

tests_diode_SOURCES = \
	tests/diode/tests.c
tests_diode_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_diode_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_diode_DEPENDENCIES = libpacketgraph-dev.la

tests_rxtx_SOURCES = \
	tests/rxtx/tests.c
tests_rxtx_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_rxtx_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_rxtx_DEPENDENCIES = libpacketgraph-dev.la

tests_pmtud_SOURCES = \
	tests/pmtud/tests.c
tests_pmtud_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_pmtud_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_pmtud_DEPENDENCIES = libpacketgraph-dev.la

tests_ip_fragment_SOURCES = \
	tests/ip-fragment/tests.c
tests_ip_fragment_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_ip_fragment_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_ip_fragment_DEPENDENCIES = libpacketgraph-dev.la

tests_firewall_SOURCES = \
	tests/firewall/test-icmp.c\
	tests/firewall/tests.c\
	tests/firewall/test-tcp.c
tests_firewall_CFLAGS = $(libpacketgraph_dev_la_CFLAGS) -Wno-address-of-packed-member
tests_firewall_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_firewall_DEPENDENCIES = libpacketgraph-dev.la

tests_integration_SOURCES = \
	tests/integration/tests.c
tests_integration_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_integration_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_integration_DEPENDENCIES = libpacketgraph-dev.la

tests_nic_SOURCES = \
	tests/nic/test-nic.c\
	tests/nic/tests.c
tests_nic_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_nic_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_nic_DEPENDENCIES = libpacketgraph-dev.la

tests_print_SOURCES = \
	tests/print/tests.c
tests_print_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_print_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_print_DEPENDENCIES = libpacketgraph-dev.la

tests_queue_SOURCES = \
	tests/queue/tests.c
tests_queue_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_queue_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_queue_DEPENDENCIES = libpacketgraph-dev.la

tests_switch_SOURCES = \
	tests/switch/tests.c
tests_switch_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_switch_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_switch_DEPENDENCIES = libpacketgraph-dev.la

tests_vhost_SOURCES = \
	tests/vhost/tests.c\
	tests/vhost/test-vhost.c
tests_vhost_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_vhost_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_vhost_DEPENDENCIES = libpacketgraph-dev.la

tests_vtep_SOURCES = \
	tests/vtep/tests.c
tests_vtep_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_vtep_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_vtep_DEPENDENCIES = libpacketgraph-dev.la

tests_tap_SOURCES = \
	tests/tap/tests.c
tests_tap_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_tap_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_tap_DEPENDENCIES = libpacketgraph-dev.la

tests_thread_SOURCES = \
	tests/thread/tests.c
tests_thread_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_thread_LDFLAGS = libpacketgraph-dev.la
EXTRA_tests_thread_DEPENDENCIES = libpacketgraph-dev.la

TESTS = \
	tests/antispoof/test.sh\
	tests/core/test.sh\
	tests/diode/test.sh\
	tests/rxtx/test.sh\
	tests/pmtud/test.sh\
	tests/ip-fragment/test.sh\
	tests/firewall/test.sh\
	tests/nic/test.sh\
	tests/print/test.sh\
	tests/queue/test.sh\
	tests/switch/test.sh\
	tests/vtep/test.sh\
	tests/tap/test.sh\
	tests/thread/test.sh\
	tests/integration/test.sh\
	tests/vhost/test.sh

noinst_PROGRAMS = 

if PG_BENCHMARKS
noinst_PROGRAMS += bench-antispoof bench-core bench-diode bench-rxtx bench-firewall bench-nic bench-print bench-pmtud bench-queue bench-switch bench-vhost bench-vtep bench-tap bench-ip-fragment

bench_antispoof_SOURCES = \
	tests/antispoof/bench.c\
	tests/antispoof/bench-antispoof.c
bench_antispoof_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_antispoof_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_antispoof_DEPENDENCIES = libpacketgraph-dev.la

bench_core_SOURCES = \
	tests/core/bench-hub.c\
	tests/core/bench-nop.c\
	tests/core/bench.c
bench_core_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_core_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_core_DEPENDENCIES = libpacketgraph-dev.la

bench_diode_SOURCES = \
	tests/diode/bench.c\
	tests/diode/bench-diode.c
bench_diode_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_diode_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_diode_DEPENDENCIES = libpacketgraph-dev.la

bench_rxtx_SOURCES = \
	tests/rxtx/bench.c\
	tests/rxtx/bench-rxtx.c
bench_rxtx_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_rxtx_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_rxtx_DEPENDENCIES = libpacketgraph-dev.la

bench_pmtud_SOURCES = \
	tests/pmtud/bench.c
bench_pmtud_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_pmtud_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_pmtud_DEPENDENCIES = libpacketgraph-dev.la

bench_ip_fragment_SOURCES = \
	tests/ip-fragment/bench.c
bench_ip_fragment_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_ip_fragment_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_ip_fragment_DEPENDENCIES = libpacketgraph-dev.la

bench_firewall_SOURCES = \
	tests/firewall/bench.c\
	tests/firewall/bench-firewall.c
bench_firewall_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_firewall_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_firewall_DEPENDENCIES = libpacketgraph-dev.la

bench_nic_SOURCES = \
	tests/nic/bench.c\
	tests/nic/bench-nic.c
bench_nic_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_nic_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_nic_DEPENDENCIES = libpacketgraph-dev.la

bench_print_SOURCES = \
	tests/print/bench-print.c\
	tests/print/bench.c
bench_print_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_print_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_print_DEPENDENCIES = libpacketgraph-dev.la

bench_queue_SOURCES = \
	tests/queue/bench-queue.c\
	tests/queue/bench.c
bench_queue_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_queue_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_queue_DEPENDENCIES = libpacketgraph-dev.la

bench_switch_SOURCES = \
	tests/switch/bench-switch.c\
	tests/switch/bench.c
bench_switch_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_switch_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_switch_DEPENDENCIES = libpacketgraph-dev.la

bench_vhost_SOURCES = \
	tests/vhost/bench-vhost.c\
	tests/vhost/bench.c
bench_vhost_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_vhost_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_vhost_DEPENDENCIES = libpacketgraph-dev.la

bench_vtep_SOURCES = \
	tests/vtep/bench-vtep.c\
	tests/vtep/bench.c
bench_vtep_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_vtep_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_vtep_DEPENDENCIES = libpacketgraph-dev.la

bench_tap_SOURCES = \
	tests/tap/bench-tap.c\
	tests/tap/bench.c
bench_tap_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_tap_LDFLAGS = libpacketgraph-dev.la
EXTRA_bench_tap_DEPENDENCIES = libpacketgraph-dev.la

bench_dependencies = bench-antispoof bench-core bench-diode bench-firewall bench-nic bench-print bench-queue bench-switch bench-vhost bench-pmtud bench-vtep bench-tap bench-rxtx bench-ip-fragment
bench: $(bench_dependencies)
	$(srcdir)/tests/antispoof/bench.sh
	$(srcdir)/tests/core/bench.sh
	$(srcdir)/tests/diode/bench.sh
	$(srcdir)/tests/firewall/bench.sh
	$(srcdir)/tests/nic/bench.sh
	$(srcdir)/tests/print/bench.sh
	$(srcdir)/tests/queue/bench.sh
	$(srcdir)/tests/switch/bench.sh
	$(srcdir)/tests/vhost/bench.sh
	$(srcdir)/tests/vtep/bench.sh
	$(srcdir)/tests/rxtx/bench.sh
	$(srcdir)/tests/pmtud/bench.sh
	$(srcdir)/tests/tap/bench.sh
	$(srcdir)/tests/ip-fragment/bench.sh

benchmark.%: $(bench_dependencies)
	echo -n > $@
	$(srcdir)/tests/antispoof/bench.sh -f $* -o $@
	$(srcdir)/tests/core/bench.sh -f $* -o $@
	$(srcdir)/tests/diode/bench.sh -f $* -o $@
	$(srcdir)/tests/firewall/bench.sh -f $* -o $@
	$(srcdir)/tests/nic/bench.sh -f $* -o $@
	$(srcdir)/tests/print/bench.sh -f $* -o $@
	$(srcdir)/tests/queue/bench.sh -f $* -o $@
	$(srcdir)/tests/switch/bench.sh -f $* -o $@
	$(srcdir)/tests/vhost/bench.sh -f $* -o $@
	$(srcdir)/tests/vtep/bench.sh -f $* -o $@
	$(srcdir)/tests/rxtx/bench.sh -f $* -o $@
	$(srcdir)/tests/pmtud/bench.sh -f $* -o $@
	$(srcdir)/tests/tap/bench.sh -f $* -o $@
	$(srcdir)/tests/ip-fragment/bench.sh -f $* -o $@
endif

style:
	$(srcdir)/tests/style/test.sh $(srcdir)

help:
	@echo "make               : build packetgraph"
	@echo "make doc           : generate public documentation using doxygen"
	@echo "make style         : run style tests"
	@echo "make check         : run tests"
	@echo "make install       : install packetgraph library"
	@echo "make uninstall     : uninstall packetgraph library"
if PG_BENCHMARKS
	@echo "make bench         : run benchmarks"
	@echo "make benchmark.csv : output results to benchmark.csv"
endif
	@echo ""
	@echo "if you need to run a specific test, i.e. vtep:"
	@echo "./tests/vtep/test.sh"

if PG_EXAMPLES
noinst_PROGRAMS = example-firewall example-switch example-rxtx example-nic example-dperf

example_firewall_SOURCES = examples/firewall/firewall.c
example_firewall_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_firewall_LDFLAGS = libpacketgraph.la
EXTRA_example_firewall_DEPENDENCIES = libpacketgraph.la

example_switch_SOURCES = examples/switch/switch.c
example_switch_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_switch_LDFLAGS = libpacketgraph.la $(GLIB_LIBS)
EXTRA_example_switch_DEPENDENCIES = libpacketgraph.la

example_rxtx_SOURCES = examples/rxtx/rxtx.c
example_rxtx_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_rxtx_LDFLAGS = libpacketgraph.la $(GLIB_LIBS)
EXTRA_example_rxtx_DEPENDENCIES = libpacketgraph.la

example_nic_SOURCES = examples/nic/nic.c
example_nic_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_nic_LDFLAGS = libpacketgraph.la
EXTRA_example_nic_DEPENDENCIES = libpacketgraph.la

example_dperf_SOURCES = examples/dperf/dperf.c
example_dperf_CFLAGS = $(libpacketgraph_la_CFLAGS) -Wno-address-of-packed-member
example_dperf_LDFLAGS = libpacketgraph.la $(GLIB_LIBS)
EXTRA_example_dperf_DEPENDENCIES = libpacketgraph.la
endif

doxygen.conf: $(srcdir)/doc/doxygen.conf.template
	$(SED) "s|PG_SRC_PATH|@srcdir@|g" $< > $@

.PHONY: doc
doc: doxygen.conf
	$(srcdir)/doc/sed_readme.sh
	doxygen $^
	$(srcdir)/doc/check_error.sh
	$(srcdir)/doc/deploy_documentation.sh

# libtoolize recommanded this
ACLOCAL_AMFLAGS = -I m4
