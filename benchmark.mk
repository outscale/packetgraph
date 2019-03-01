BENCHMARK=true

.PHONY: benchclean benchfclean

################################################################################
##                                    Benchmark                               ##
################################################################################

bench_antispoof_SOURCES = \
  tests/antispoof/bench-antispoof.c\
  tests/antispoof/bench.c
bench_antispoof_OBJECTS = $(bench_antispoof_SOURCES:.c=.o)
bench_core_SOURCES = \
  tests/core/bench-hub.c\
  tests/core/bench-nop.c\
  tests/core/bench.c
bench_core_OBJECTS = $(bench_core_SOURCES:.c=.o)
bench_diode_SOURCES = \
  tests/diode/bench.c\
  tests/diode/bench-diode.c
bench_diode_OBJECTS = $(bench_diode_SOURCES:.c=.o)
bench_rxtx_SOURCES = \
  tests/rxtx/bench.c\
  tests/rxtx/bench-rxtx.c
bench_rxtx_OBJECTS = $(bench_rxtx_SOURCES:.c=.o)
bench_pmtud_SOURCES = \
  tests/pmtud/bench.c
bench_pmtud_OBJECTS = $(bench_pmtud_SOURCES:.c=.o)
bench_ip_fragment_SOURCES = \
  tests/ip-fragment/bench.c
bench_ip_fragment_OBJECTS = $(bench_ip_fragment_SOURCES:.c=.o)
bench_firewall_SOURCES = \
  tests/firewall/bench.c\
  tests/firewall/bench-firewall.c
bench_firewall_OBJECTS = $(bench_firewall_SOURCES:.c=.o)
bench_nic_SOURCES = \
  tests/nic/bench.c\
  tests/nic/bench-nic.c
bench_nic_OBJECTS = $(bench_nic_SOURCES:.c=.o)
bench_print_SOURCES = \
  tests/print/bench-print.c\
  tests/print/bench.c
bench_print_OBJECTS = $(bench_print_SOURCES:.c=.o)
bench_queue_SOURCES = \
  tests/queue/bench-queue.c\
  tests/queue/bench.c
bench_queue_OBJECTS = $(bench_queue_SOURCES:.c=.o)
bench_switch_SOURCES = \
  tests/switch/bench-switch.c\
  tests/switch/bench.c
bench_switch_OBJECTS = $(bench_switch_SOURCES:.c=.o)
bench_vhost_SOURCES = \
  tests/vhost/bench-vhost.c\
  tests/vhost/bench.c
bench_vhost_OBJECTS = $(bench_vhost_SOURCES:.c=.o)
bench_vtep_SOURCES = \
  tests/vtep/bench-vtep.c\
  tests/vtep/bench.c
bench_vtep_OBJECTS = $(bench_vtep_SOURCES:.c=.o)
bench_tap_SOURCES = \
  tests/tap/bench-tap.c\
  tests/tap/bench.c
bench_tap_OBJECTS = $(bench_tap_SOURCES:.c=.o)

bench_CFLAGS = $(PG_CFLAGS)
bench_HEADERS = $(PG_HEADERS)
bench_LDFLAGS = -lpacketgraph $(PG_LIBADD) -L$(srcdir) -ldl -lm -lnuma -lpthread -lpcap -lz

################################################################################
##                                Benchmark compile                           ##
################################################################################

bench-antispoof: $(bench_antispoof_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_antispoof_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_antispoof_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-core: $(bench_core_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_core_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_core_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-diode: $(bench_diode_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_diode_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_diode_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-firewall: $(bench_firewall_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_firewall_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_firewall_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-nic: $(bench_nic_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_nic_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_nic_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-print: $(bench_print_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_print_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_print_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-queue: $(bench_queue_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_queue_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_queue_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-switch: $(bench_switch_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_switch_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_switch_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-vhost: $(bench_vhost_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_vhost_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_vhost_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-pmtud: $(bench_pmtud_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_pmtud_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_pmtud_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-vtep: $(bench_vtep_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_vtep_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_vtep_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-tap: $(bench_tap_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_tap_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_tap_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-rxtx: $(bench_rxtx_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_rxtx_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_rxtx_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench-ip-fragment: $(bench_ip_fragment_OBJECTS)
	$(CC) $(bench_CFLAGS) $(bench_HEADERS) $(bench_ip_fragment_OBJECTS) $(bench_LDFLAGS) -o $@

$(bench_ip_fragment_OBJECTS): %.o : %.c
	$(CC) -c $(bench_CFLAGS) $(bench_HEADERS) $< -o $@

bench_compile: all bench-antispoof bench-core bench-diode bench-firewall bench-nic bench-print bench-queue bench-switch bench-vhost bench-pmtud bench-vtep bench-tap bench-rxtx bench-ip-fragment

################################################################################
#                                  Benchmark tests                             #
################################################################################

bench: bench_compile
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

benchmark.%: $(bench_compile)
	echo ">>> $@" > $@
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

benchfclean: benchclean
	rm -fv bench-antispoof bench-core bench-diode bench-rxtx bench-pmtud bench-ip-fragment bench-firewall bench-nic bench-print bench-queue bench-switch bench-vtep bench-tap bench-thread bench-integration bench-vhost

benchclean:
	rm -fv $(bench_antispoof_OBJECTS) $(bench_core_OBJECTS) $(bench_diode_OBJECTS) $(bench_rxtx_OBJECTS) $(bench_pmtud_OBJECTS) $(bench_ip_fragment_OBJECTS) $(bench_firewall_OBJECTS) $(bench_nic_OBJECTS) $(bench_print_OBJECTS) $(bench_queue_OBJECTS) $(bench_switch_OBJECTS) $(bench_vtep_OBJECTS) $(bench_tap_OBJECTS) $(bench_thread_OBJECTS) $(bench_integration_OBJECTS) $(bench_vhost_OBJECTS)
