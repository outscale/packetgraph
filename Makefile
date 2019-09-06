include config.mk
include npf.mk
include tests.mk

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
			 src/seccomp.c\
			 src/ip-fragment.c
PG_OBJECTS = $(PG_SOURCES:.c=.o)
PG_dev_OBJECTS = $(PG_SOURCES:.c=-dev.o)

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

PG_LIBADD = $(RTE_SDK_LIBS) $(GLIB_LIBS)
	#FIXME '^pg_[^_]' does not take all symbols needed (i.e. __pg_error_*)

PG_LDFLAGS = -version-info 17:5:0 -export-symbols-regex 'pg_[^_]' -no-undefined --export-all-symbols $(PG_COV_LFLAGS)

PG_CFLAGS = $(EXTRA_CFLAGS) -march=$(PG_MARCH) -fmessage-length=0 -Werror -Wall -Wextra -Winit-self -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11 $(GLIB_CFLAGS) $(RTE_SDK_CFLAGS) $(PG_ASAN_CFLAGS) -Wno-implicite-fallthrough -Wno-unknown-warning-option -Wno-deprecated-declarations -Wno-address-of-packed-member $(PG_COV_CFLAGS)

PG_dev_CFLAGS = $(PG_CFLAGS) -D PG_NIC_STUB -D PG_NIC_BENCH -D PG_QUEUE_BENCH -D PG_VHOST_BENCH -D PG_RXTX_BENCH -D PG_TAP_BENCH -D PG_MALLOC_DEBUG

dist_doc_DATA = README.md

check_PROGRAMS = tests-antispoof tests-core tests-diode tests-rxtx tests-firewall tests-integration tests-nic tests-print tests-queue tests-switch tests-vhost tests-vtep  tests-pmtud tests-tap tests-ip-fragment tests-thread

ACLOCAL_AMFLAGS = -I m4

.DEFAULT_GOAL := all

.PHONY: doc style check clean fclean help all install uninstall cov

cov:
	$(srcdir)tests_coverage.sh

all: dev $(PG_NAME)

$(PG_NAME): lpm cdb nvlist thmap qsbr sljit bpfjit npf npfkern $(PG_OBJECTS)
	ar rcv $(PG_NAME).a $(PG_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(nvlist_OBJECTS) $(thmap_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS)
	$(CC) -shared -Wl,-soname,$(PG_NAME).so.17 -o $(PG_NAME).so.17.5.0 $(PG_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS) -lc
	echo $(PG_NAME)" compiled"

$(PG_OBJECTS) : src/%.o : src/%.c
	$(CC) -c $(PG_CFLAGS) $(PG_HEADERS) $< -o $@

$(PG_dev_OBJECTS): src/%-dev.o : src/%.c
	$(CC) -c $(PG_dev_CFLAGS) $(PG_HEADERS) $< -o $@

doxygen.conf: $(srcdir)/doc/doxygen.conf.template
	$(shell sed "s|PG_SRC_PATH|$(srcdir)|g" $< > $@)

doc: doxygen.conf
	$(srcdir)/doc/sed_readme.sh
	doxygen $^
	$(srcdir)/doc/check_error.sh
	$(srcdir)/doc/deploy_documentation.sh

style:
	$(srcdir)/tests/style/test.sh $(srcdir)

check:
	$(srcdir)/run_tests.sh

dev: lpm cdb nvlist thmap qsbr sljit bpfjit npf npfkern $(PG_dev_OBJECTS)
	ar rcv $(PG_NAME)-dev.a $(PG_dev_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(nvlist_OBJECTS) $(thmap_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS)
	$(CC) -shared -Wl,-soname,$(PG_NAME)-dev.so.17 -o $(PG_NAME)-dev.so.17.5.0 $(PG_dev_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS) -lc
	echo "$(PG_CFLAGS)-dev compiled"

clean: clean_npf testcleanobj
	@rm -fv $(PG_OBJECTS)
	@rm -fv $(PG_dev_OBJECTS)
	@rm -Rfv dev
ifdef BENCHMARK_INCLUDED
	$(MAKE) benchclean
endif
ifdef EXAMPLE_INCLUDED
	$(MAKE) exampleclean
endif
ifdef PG_COV_CFLAGS
	@rm -fv $(srcdir)src/*.gcno
	@rm -fv $(srcdir)src/*.gcda
	@rm -fv $(srcdir)tests/*/*.gcda
	@rm -fv $(srcdir)tests/*/*.gcno
endif

fclean: clean fclean_npf testclean
	@rm -fv $(PG_NAME).a
	@rm -fv $(PG_dev_NAME).a
	@rm -fv $(PG_NAME).so.17.5.0
	@rm -fv $(PG_dev_NAME).so.17.5.0
ifdef BENCHMARK
	$(MAKE) benchfclean
endif
ifdef EXAMPLE
	$(MAKE) examplefclean
endif
ifdef PG_COV_CFLAGS
	@rm -fv $(srcdir)/src/*.gcno
	@rm -fv $(srcdir)/src/*.gcda
	@rm -fv $(srcdir)tests/*/*.gcda
	@rm -fv $(srcdir)tests/*/*.gcno
endif


install: $(PG_NAME)
	mkdir -p $(PREFIX)/lib/
	mkdir -p $(PREFIX)/include/
	@cp -v $(PG_NAME).a $(PREFIX)/lib/
	@cp -v $(PG_NAME).so* $(PREFIX)/lib/
	@cp -v $(PG_NAME)-dev.a* $(PREFIX)/lib/
	@cp -v $(PG_NAME)-dev.so* $(PREFIX)/lib/
	@cp -v libbpfjit.a $(PREFIX)/lib/
	@cp -v libcdb.a $(PREFIX)/lib/
	@cp -v liblpm.a $(PREFIX)/lib/
	@cp -v libnpf.a $(PREFIX)/lib/
	@cp -v libnpfkern.a $(PREFIX)/lib/
	@cp -v libnvlist.a $(PREFIX)/lib/
	@cp -v libqsbr.a $(PREFIX)/lib/
	@cp -v libsljit.a $(PREFIX)/lib/
	@cp -v libthmap.a $(PREFIX)/lib/
	@cp -vr include/ $(PREFIX)/

uninstall:
	@rm -fv $(PREFIX)/lib/$(PG_NAME)* # what could posibly go wrong ?
	@rm -fv $(PREFIX)/lib/libbpfjit.a
	@rm -fv $(PREFIX)/lib/libcdb.a
	@rm -fv $(PREFIX)/lib/liblpm.a
	@rm -fv $(PREFIX)/lib/libnpf.a
	@rm -fv	$(PREFIX)/lib/libnpfkern.a
	@rm -fv	$(PREFIX)/lib/libnvlist.a
	@rm -fv	$(PREFIX)/lib/libqsbr.a
	@rm -fv	$(PREFIX)/lib/libsljit.a
	@rm -fv	$(PREFIX)/lib/libthmap.a
	@rm -rvf $(PREFIX)/include/packetgraph/

help:
	@echo "make               : build packetgraph"
	@echo "make doc           : generate public documentation using doxygen"
	@echo "make style         : run style tests"
	@echo "make check         : run tests"
ifdef BENCHMARK
	@echo "make bench         : run benchmarks"
	@echo "make benchmark.csv : output results to benchmark.csv"
endif
	@echo ""
	@echo "if you need to run a specific test, i.e. vtep:"
	@echo "./tests/vtep/test.sh"
