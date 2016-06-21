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
	src/rxtx.c\
	src/npf/npf/dpdk/npf_dpdk.c\
	src/antispoof.c\
	src/collect.c\
	src/vtep.c\
	src/nic.c\
	src/tap.c\
	src/graph.c\
	src/diode.c\
	src/switch.c\
	src/pmtud.c

pkginclude_HEADERS = \
	include/packetgraph/common.h\
	include/packetgraph/print.h\
	include/packetgraph/hub.h\
	include/packetgraph/packetgraph.h\
	include/packetgraph/diode.h\
	include/packetgraph/nop.h\
	include/packetgraph/nic.h\
	include/packetgraph/tap.h\
	include/packetgraph/antispoof.h\
	include/packetgraph/brick.h\
	include/packetgraph/lifecycle.h\
	include/packetgraph/graph.h\
	include/packetgraph/vtep.h\
	include/packetgraph/firewall.h\
	include/packetgraph/vhost.h\
	include/packetgraph/rxtx.h\
	include/packetgraph/switch.h\
	include/packetgraph/queue.h\
	include/packetgraph/pmtud.h\
	include/packetgraph/errors.h

libpacketgraph_la_LIBADD = $(RTE_SDK_LIBS) $(GLIB_LIBS)
# FIXME '^pg_[^_]' does not take all symbols needed (i.e. __pg_error_*)
libpacketgraph_la_LDFLAGS = -version-info 0:0:0 -export-symbols-regex 'pg_[^_]' -no-undefined $(RTE_SDK_LDFLAGS)
libpacketgraph_la_CFLAGS = -Ofast -march=core-avx-i -mtune=core-avx-i -fmessage-length=0 -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11 $(GLIB_CFLAGS) $(RTE_SDK_CFLAGS) -I$(srcdir)/include -I$(srcdir)/src

noinst_LTLIBRARIES =

if PG_BUILD_NPF
noinst_LTLIBRARIES += libprop.la libcdb.la libqsbr.la libsljit.la libbpfjit.la libnpf.la libnpfkern.la

libprop_la_SOURCES = \
	src/npf/proplib/src/prop_array.c\
	src/npf/proplib/src/prop_array_util.c\
	src/npf/proplib/src/prop_bool.c\
	src/npf/proplib/src/prop_data.c\
	src/npf/proplib/src/prop_dictionary.c\
	src/npf/proplib/src/prop_dictionary_util.c\
	src/npf/proplib/src/prop_ingest.c\
	src/npf/proplib/src/prop_number.c\
	src/npf/proplib/src/prop_object.c\
	src/npf/proplib/src/prop_stack.c\
	src/npf/proplib/src/prop_string.c\
	src/npf/proplib/src/prop_zlib.c\
	src/npf/proplib/src/rb.c
libprop_la_includes = -I$(srcdir)/src/npf/proplib/include
libprop_la_CFLAGS = $(libprop_la_includes) -pipe -Werror -Wall -Wextra -Wvla -Wno-overlength-strings -Wunsafe-loop-optimizations -Wundef -Wformat=2 -Wsign-compare -Wformat-security -Wmissing-include-dirs -Wformat-nonliteral -Wold-style-definition -Wpointer-arith -Winit-self -Wdeclaration-after-statement -Wfloat-equal -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-declarations -Wmissing-noreturn -Wshadow -Wendif-labels -Wcast-align -Wstrict-aliasing -Wwrite-strings -Wno-unused-parameter -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector-all -Ofast -std=gnu99 -fPIC

libcdb_la_SOURCES = \
	src/npf/libcdb/src/cdbr.c\
	src/npf/libcdb/src/cdbw.c\
	src/npf/libcdb/src/mi_vector_hash.c
libcdb_la_includes = -I$(srcdir)/src/npf/libcdb/src
libcdb_la_CFLAGS = $(libcdb_la_includes) -D__RCSID\(x\)= -std=gnu99 -O2 -W -Wextra -Werror -DNDEBUG -fPIC

libqsbr_la_SOURCES = \
	src/npf/libqsbr/src/ebr.c\
	src/npf/libqsbr/src/gc.c\
	src/npf/libqsbr/src/qsbr.c
libqsbr_la_includes = -I$(srcdir)/src/npf/libqsbr
libqsbr_la_CFLAGS = $(libqsbr_la_includes) -D__RCSID\(x\)= -fPIC

libsljit_la_SOURCES = \
	src/npf/sljit/regexJIT.c\
	src/npf/sljit/sljitLir.c
libsljit_includes = -I$(srcdir)/src/npf/sljit
libsljit_la_CFLAGS = -DSLJIT_CONFIG_AUTO=1 -DSLJIT_VERBOSE=0 -DSLJIT_DEBUG=0 -Ofast -fPIC

libbpfjit_la_SOURCES = src/npf/bpfjit/bpfjit.c
libbpfjit_includes = -I$(srcdir)/src/npf/bpfjit
libbpfjit_la_CFLAGS = $(libsljit_includes) -DSLJIT_CONFIG_AUTO=1 -DSLJIT_VERBOSE=0 -DSLJIT_DEBUG=0 -Ofast -fPIC
libbpfjit_la_LIBADD = libsljit.la

libnpf_la_SOURCES = src/npf/npf/src/libnpf/npf.c
libnpf_la_includes = -I$(srcdir)/src/npf/npf/src/libnpf -I$(srcdir)/src/npf/npf/src/kern/stand
libnpf_la_CFLAGS = $(libnpf_la_includes) $(libprop_la_includes) $(libqsbr_la_includes) $(libcdb_la_includes) -std=gnu99 -O2 -g -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -D__KERNEL_RCSID\(x,y\)= -D_NPF_STANDALONE -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused -DNDEBUG -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE -fPIC

libnpfkern_la_SOURCES = \
	src/npf/npf/src/kern/npf.c\
	src/npf/npf/src/kern/npf_conf.c\
	src/npf/npf/src/kern/npf_ctl.c\
	src/npf/npf/src/kern/npf_handler.c\
	src/npf/npf/src/kern/npf_mbuf.c\
	src/npf/npf/src/kern/npf_bpf.c\
	src/npf/npf/src/kern/npf_ruleset.c\
	src/npf/npf/src/kern/npf_rproc.c\
	src/npf/npf/src/kern/npf_tableset.c\
	src/npf/npf/src/kern/npf_tableset_ptree.c\
	src/npf/npf/src/kern/npf_if.c\
	src/npf/npf/src/kern/npf_inet.c\
	src/npf/npf/src/kern/npf_conn.c\
	src/npf/npf/src/kern/npf_conndb.c\
	src/npf/npf/src/kern/npf_state.c\
	src/npf/npf/src/kern/npf_state_tcp.c\
	src/npf/npf/src/kern/npf_nat.c\
	src/npf/npf/src/kern/npf_alg.c\
	src/npf/npf/src/kern/npf_sendpkt.c\
	src/npf/npf/src/kern/npf_worker.c\
	src/npf/npf/src/kern/stand/bpf_filter.c\
	src/npf/npf/src/kern/stand/rb.c\
	src/npf/npf/src/kern/stand/ptree.c\
	src/npf/npf/src/kern/stand/murmurhash.c\
	src/npf/npf/src/kern/stand/subr_hash.c\
	src/npf/npf/src/kern/stand/mempool.c\
	src/npf/npf/src/kern/stand/tls_pth.c
libnpfkern_la_includes = -I$(srcdir)/src/npf/npf/src/kern/stand
libnpfkern_la_CFLAGS = $(libcdb_la_includes) $(libprop_la_includes) $(libqsbr_la_includes) $(libnpf_la_includes) $(libnpfkern_la_includes) $(libbpfjit_includes) -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_NPF_STANDALONE -D__KERNEL_RCSID\(x,y\)= -std=gnu99 -O2 -flto -g -Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused -fvisibility=hidden -DNDEBUG -fPIC -D__RCSID\(x\)=
# Trick to avoid symbol conflicts
libnpfkern_la_CFLAGS += -Dnpf_ruleset_add=_npf_ruleset_add -Dnpf_ruleset_remove=_npf_ruleset_remove -Dnpf_ruleset_remkey=_npf_ruleset_remkey -Dnpf_ruleset_flush=_npf_ruleset_flush -Dnpf_rule_setcode=_npf_rule_setcode -Dnpf_rule_getid=_npf_rule_getid -Dnpf_ext_construct=_npf_ext_construct -Dnpf_rproc_create=_npf_rproc_create -Dnpf_table_create=_npf_table_create -Dnpf_table_destroy=_npf_table_destroy -Dnpf_table_insert=_npf_table_insert -Drb_tree_init=_rb_tree_init -Drb_tree_find_node=_rb_tree_find_node -Drb_tree_find_node_geq=_rb_tree_find_node_geq -Drb_tree_find_node_leq=_rb_tree_find_node_leq -Drb_tree_insert_node=_rb_tree_insert_node -Drb_tree_iterate=_rb_tree_iterate -Drb_tree_remove_node=_rb_tree_remove_node
libnpfkern_la_LIBADD = libqsbr.la libcdb.la libprop.la libbpfjit.la -ljemalloc -lpthread -lpcap

libpacketgraph_la_LIBADD += libnpf.la libnpfkern.la
libpacketgraph_la_CFLAGS += $(libnpf_la_includes) $(libnpfkern_la_includes) $(libprop_la_includes)
else
libpacketgraph_la_LIBADD += -lnpf -lnpfkern
endif

dist_doc_DATA = README.md

check_PROGRAMS = tests-antispoof tests-core tests-diode tests-rxtx tests-firewall tests-integration tests-nic tests-print tests-queue tests-switch tests-vhost tests-vtep  tests-pmtud tests-tap

noinst_LTLIBRARIES += libpacketgraph-dev.la
libpacketgraph_dev_la_SOURCES = $(libpacketgraph_la_SOURCES)
libpacketgraph_dev_la_LIBADD = $(libpacketgraph_la_LIBADD)
libpacketgraph_dev_la_LDFLAGS = -no-undefined --export-all-symbols $(RTE_SDK_LDFLAGS)
libpacketgraph_dev_la_CFLAGS = -g $(libpacketgraph_la_CFLAGS) -D PG_NIC_STUB -D PG_NIC_BENCH -D PG_QUEUE_BENCH -D PG_VHOST_BENCH -D PG_RXTX_BENCH -D PG_TAP_BENCH

tests_antispoof_SOURCES = \
	tests/antispoof/test-arp-gratuitous.c\
	tests/antispoof/test-arp-response.c\
	tests/antispoof/test-rarp.c\
	tests/antispoof/tests.c
tests_antispoof_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_antispoof_LDADD = libpacketgraph-dev.la

tests_core_SOURCES = \
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

tests_diode_SOURCES = \
	tests/diode/tests.c
tests_diode_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_diode_LDFLAGS = libpacketgraph-dev.la


tests_rxtx_SOURCES = \
	tests/rxtx/tests.c
tests_rxtx_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_rxtx_LDFLAGS = libpacketgraph-dev.la

tests_pmtud_SOURCES = \
	tests/pmtud/tests.c
tests_pmtud_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_pmtud_LDFLAGS = libpacketgraph-dev.la

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

tests_queue_SOURCES = \
	tests/queue/tests.c
tests_queue_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_queue_LDFLAGS = libpacketgraph-dev.la

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

tests_tap_SOURCES = \
	tests/tap/tests.c
tests_tap_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
tests_tap_LDFLAGS = libpacketgraph-dev.la

TESTS = \
	tests/antispoof/test.sh\
	tests/core/test.sh\
	tests/diode/test.sh\
	tests/rxtx/test.sh\
	tests/pmtud/test.sh\
	tests/firewall/test.sh\
	tests/integration/test.sh\
	tests/nic/test.sh\
	tests/print/test.sh\
	tests/queue/test.sh\
	tests/switch/test.sh\
	tests/vhost/test.sh\
	tests/vtep/test.sh\
	tests/tap/test.sh

noinst_PROGRAMS = 

if PG_BENCHMARKS
noinst_PROGRAMS += bench-antispoof bench-core bench-diode bench-rxtx bench-firewall bench-nic bench-print bench-pmtud bench-queue bench-switch bench-vhost bench-vtep bench-tap

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

bench_rxtx_SOURCES = \
	tests/rxtx/bench.c\
	tests/rxtx/bench-rxtx.c
bench_rxtx_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_rxtx_LDFLAGS = libpacketgraph-dev.la

bench_pmtud_SOURCES = \
	tests/pmtud/bench.c
bench_pmtud_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_pmtud_LDFLAGS = libpacketgraph-dev.la

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

bench_queue_SOURCES = \
	tests/queue/bench-queue.c\
	tests/queue/bench.c
bench_queue_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_queue_LDFLAGS = libpacketgraph-dev.la

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

bench_tap_SOURCES = \
	tests/tap/bench-tap.c\
	tests/tap/bench.c
bench_tap_CFLAGS = $(libpacketgraph_dev_la_CFLAGS)
bench_tap_LDFLAGS = libpacketgraph-dev.la

bench: bench-antispoof bench-core bench-diode bench-firewall bench-nic bench-print bench-queue bench-switch bench-vhost bench-pmtud bench-vtep bench-tap bench-rxtx
	$(srcdir)/tests/antispoof/bench.sh
	$(srcdir)/tests/core/bench.sh
	$(srcdir)/tests/diode/bench.sh
	$(srcdir)/tests/firewall/bench.sh
	$(srcdir)/tests/nic/bench.sh
	$(srcdir)/tests/print/bench.sh
	$(srcdir)/tests/queue/bench.sh
	$(srcdir)/tests/switch/bench.sh
	$(srcdir)/tests/vtep/bench.sh
	$(srcdir)/tests/rxtx/bench.sh
	$(srcdir)/tests/pmtud/bench.sh
	$(srcdir)/tests/tap/bench.sh
endif

style:
	$(srcdir)/tests/style/test.sh $(srcdir)

help:
	@echo "make           : build packetgraph"
	@echo "make doc       : generate public documentation using doxygen"
	@echo "make style     : run style tests"
	@echo "make check     : run tests"
	@echo "make install   : install packetgraph library"
	@echo "make uninstall : uninstall packetgraph library"
if PG_BENCHMARKS
	@echo "make bench     : run benchmarks"
endif
	@echo ""
	@echo "if you need to run a specific test, i.e. vtep:"
	@echo "./tests/vtep/test.sh"

if PG_EXAMPLES
noinst_PROGRAMS += example-firewall example-switch example-rxtx

example_firewall_SOURCES = \
	examples/firewall/firewall.c
example_firewall_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_firewall_LDFLAGS = libpacketgraph.la

# We add libpacketgraph_la_LIBADD because of a dependency bug we saw on
# Travis. Like this post (see below) explains, this may be due to a patched
# version of libtool in Debian/Ubuntu.
# https://www.midnight-commander.org/ticket/3540#comment:8
example_switch_SOURCES = \
	examples/switch/switch.c
example_switch_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_switch_LDFLAGS = libpacketgraph.la $(libpacketgraph_la_LIBADD)

example_rxtx_SOURCES = \
	examples/rxtx/rxtx.c
example_rxtx_CFLAGS = $(libpacketgraph_la_CFLAGS)
example_rxtx_LDFLAGS = libpacketgraph.la $(libpacketgraph_la_LIBADD)
endif

doxygen.conf: $(srcdir)/doc/doxygen.conf.template
	$(SED) "s|PG_SRC_PATH|@srcdir@|g" $< > $@

.PHONY: doc
doc: doxygen.conf
	doxygen $^

# libtoolize recommanded this
ACLOCAL_AMFLAGS = -I m4
