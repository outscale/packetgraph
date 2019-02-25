.DEFAULT_GOAL := all

include config.mk
include var.mk
include npf.mk
include tests.mk

.PHONY: doc style check clean fclean help

all: lpm cdb nvlist thmap qsbr sljit bpfjit npf npfkern $(PG_OBJECTS)
	ar rcv $(PG_NAME).a $(PG_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(nvlist_OBJECTS) $(thmap_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS)
	gcc -shared -Wl,-soname,$(PG_NAME).so.17 -o $(PG_NAME).so.17.5.0 $(PG_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS) -lc
	echo "PacketGraph compiled"

$(PG_OBJECTS) : src/%.o : src/%.c
	$(CC) -c $(PG_CFLAGS) $(PG_HEADERS) $< -o $@

doxygen.conf: $(srcdir)/doc/doxygen.conf.template
	$(SED) "s|PG_SRC_PATH|@srcdir@|g" $< > $@

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
	ar rcv $(PG_dev_NAME).a $(PG_dev_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(nvlist_OBJECTS) $(thmap_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS)
	gcc -shared -Wl,-soname,$(PG_dev_NAME).so.17 -o $(PG_dev_NAME).so.17.5.0 $(PG_dev_OBJECTS) $(lpm_OBJECTS) $(cdb_OBJECTS) $(qsbr_OBJECTS) $(sljit_OBJECTS) $(bpfjit_OBJECTS) $(npf_OBJECTS) $(npfkern_OBJECTS) -lc
	echo "PacketGraph-dev compiled"

$(PG_dev_OBJECTS): dev/%.o : src/%.c
	mkdir -pv `dirname $@`
	$(CC) -c $(PG_dev_CFLAGS) $(PG_dev_HEADERS) $< -o $@

clean: clean_npf testcleanobj
	rm -fv $(PG_OBJECTS)
	rm -fv $(PG_dev_OBJECTS)
	rm -Rfv dev
ifdef BENCHMARK_INCLUDED
	$(MAKE) benchclean
endif
ifdef EXAMPLE_INCLUDED
	$(MAKE) exampleclean
endif

fclean: clean fclean_npf testclean
	rm -fv $(PG_NAME).a
	rm -fv $(PG_dev_NAME).a
	rm -fv $(PG_NAME).so.17.5.0
	rm -fv $(PG_dev_NAME).so.17.5.0
ifdef BENCHMARK
	$(MAKE) benchfclean
endif
ifdef EXAMPLE
	$(MAKE) examplefclean
endif

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
