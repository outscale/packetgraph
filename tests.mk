tests_antispoof_DIR = tests/antispoof
tests_antispoof_SOURCES = \
	$(tests_antispoof_DIR)/test-arp-gratuitous.c\
	$(tests_antispoof_DIR)/test-arp-response.c\
	$(tests_antispoof_DIR)/test-rarp.c\
	$(tests_antispoof_DIR)/tests.c
tests_antispoof_OBJECTS = $(tests_antispoof_SOURCES:.c=.o)

tests_core_DIR = tests/core
tests_core_SOURCES = \
	$(tests_core_DIR)/test-bitmask.c\
	$(tests_core_DIR)/test-core.c\
	$(tests_core_DIR)/test-dot.c\
	$(tests_core_DIR)/test-flow.c\
	$(tests_core_DIR)/test-error.c\
	$(tests_core_DIR)/test-mac.c\
	$(tests_core_DIR)/test-pkts-count.c\
	$(tests_core_DIR)/test-graph.c\
	$(tests_core_DIR)/test-hub.c\
	$(tests_core_DIR)/tests.c
tests_core_OBJECTS = $(tests_core_SOURCES:.c=.o)

tests_diode_DIR = tests/diode
tests_diode_SOURCES = \
	$(tests_diode_DIR)/tests.c
tests_diode_OBJECTS = $(tests_diode_SOURCES:.c=.o)

tests_rxtx_DIR = tests/rxtx
tests_rxtx_SOURCES = \
	$(tests_rxtx_DIR)/tests.c
tests_rxtx_OBJECTS = $(tests_rxtx_SOURCES:.c=.o)

tests_pmtud_DIR = tests/pmtud
tests_pmtud_SOURCES = \
	$(tests_pmtud_DIR)/tests.c
tests_pmtud_OBJECTS = $(tests_pmtud_SOURCES:.c=.o)

tests_ip-fragment_DIR = tests/ip-fragment
tests_ip-fragment_SOURCES = \
	$(tests_ip-fragment_DIR)/tests.c
tests_ip-fragment_OBJECTS = $(tests_ip-fragment_SOURCES:.c=.o)

tests_integration_DIR = tests/integration
tests_integration_SOURCES = \
	$(tests_integration_DIR)/tests.c
tests_integration_OBJECTS = $(tests_integration_SOURCES:.c=.o)

tests_nic_DIR = tests/nic
tests_nic_SOURCES = \
	$(tests_nic_DIR)/test-nic.c\
	$(tests_nic_DIR)/tests.c
tests_nic_OBJECTS = $(tests_nic_SOURCES:.c=.o)

tests_print_DIR = tests/print
tests_print_SOURCES = \
	$(tests_print_DIR)/tests.c
tests_print_OBJECTS = $(tests_print_SOURCES:.c=.o)

tests_queue_DIR = tests/queue
tests_queue_SOURCES = \
	$(tests_queue_DIR)/tests.c
tests_queue_OBJECTS = $(tests_queue_SOURCES:.c=.o)

tests_switch_DIR = tests/switch
tests_switch_SOURCES = \
	$(tests_switch_DIR)/tests.c
tests_switch_OBJECTS = $(tests_switch_SOURCES:.c=.o)

tests_vhost_DIR = tests/vhost
tests_vhost_SOURCES = \
	$(tests_vhost_DIR)/tests.c\
	$(tests_vhost_DIR)/test-vhost.c
tests_vhost_OBJECTS = $(tests_vhost_SOURCES:.c=.o)

tests_vtep_DIR = tests/vtep
tests_vtep_SOURCES = \
	$(tests_vtep_DIR)/tests.c
tests_vtep_OBJECTS = $(tests_vtep_SOURCES:.c=.o)

tests_tap_DIR = tests/tap
tests_tap_SOURCES = \
	$(tests_tap_DIR)/tests.c
tests_tap_OBJECTS = $(tests_tap_SOURCES:.c=.o)

tests_thread_DIR = tests/thread
tests_thread_SOURCES = \
	$(tests_thread_DIR)/tests.c
tests_thread_OBJECTS = $(tests_thread_SOURCES:.c=.o)

tests_firewall_DIR = tests/firewall
tests_firewall_SOURCES = \
	$(tests_firewall_DIR)/test-icmp.c\
	$(tests_firewall_DIR)/tests.c\
	$(tests_firewall_DIR)/test-tcp.c
tests_firewall_OBJECTS = $(tests_firewall_SOURCES:.c=.o)

tests_CFLAGS = $(PG_dev_CFLAGS) $(PG_HEADERS)
tests_firewall_CFLAGS = $(tests_CFLAGS) -Wno-address-of-packed-member
tests_LIBS = $(PG_NAME)-dev.a $(PG_LIBADD) -L$(srcdir)/ -ldl -lm -lnuma -lpthread -lpcap -lz

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

################################################################################
##                           Tests compilation rules                          ##
################################################################################

test: dev tests-all-build antispoof core diode rxtx pmtud ip-fragment firewall nic print queue switch vtep tap thread integration vhost
	@echo "tests ended"

tests_compile: dev tests-antispoof tests-core tests-diode tests-rxtx tests-pmtud tests-ip-fragment tests-integration tests-nic tests-print tests-queue tests-switch tests-vhost tests-vtep tests-tap tests-thread tests-firewall
	@echo "Compilation done"

tests-antispoof: dev $(tests_antispoof_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_antispoof_OBJECTS) $(tests_LIBS) -o $@

$(tests_antispoof_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-core: dev $(tests_core_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_core_OBJECTS) $(tests_LIBS) -o $@

$(tests_core_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-diode: dev $(tests_diode_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_diode_OBJECTS) $(tests_LIBS) -o $@

$(tests_diode_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-rxtx: dev $(tests_rxtx_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_rxtx_OBJECTS) $(tests_LIBS) -o $@

$(tests_rxtx_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-pmtud: dev $(tests_pmtud_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_pmtud_OBJECTS) $(tests_LIBS) -o $@

$(tests_pmtud_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-ip-fragment: dev $(tests_ip-fragment_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_ip-fragment_OBJECTS) $(tests_LIBS) -o $@

$(tests_ip-fragment_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-integration: dev $(tests_integration_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_integration_OBJECTS) $(tests_LIBS) -o $@

$(tests_integration_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-nic: dev $(tests_nic_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_nic_OBJECTS) $(tests_LIBS) -o $@

$(tests_nic_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-print: dev $(tests_print_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_print_OBJECTS) $(tests_LIBS) -o $@

$(tests_print_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-queue: dev $(tests_queue_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_queue_OBJECTS) $(tests_LIBS) -o $@

$(tests_queue_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-switch: dev $(tests_switch_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_switch_OBJECTS) $(tests_LIBS) -o $@

$(tests_switch_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-vhost: dev $(tests_vhost_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_vhost_OBJECTS) $(tests_LIBS) -o $@

$(tests_vhost_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-vtep: dev $(tests_vtep_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_vtep_OBJECTS) $(tests_LIBS) -o $@

$(tests_vtep_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-tap: dev $(tests_tap_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_tap_OBJECTS) $(tests_LIBS) -o $@

$(tests_tap_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-thread: dev $(tests_thread_OBJECTS)
	$(CC) $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_thread_OBJECTS) $(tests_LIBS) -o $@

$(tests_thread_OBJECTS): %.o : %.c
	$(CC) -c $(tests_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@

tests-firewall: dev $(tests_firewall_OBJECTS)
	$(CC) $(tests_firewall_CFLAGS) $(PG_ASAN_CFLAGS) $(tests_firewall_OBJECTS) $(tests_LIBS) -o $@

$(tests_firewall_OBJECTS): %.o : %.c
	$(CC) -c $(tests_firewall_CFLAGS) $(PG_ASAN_CFLAGS) $< -o $@


tests-all-build: tests-antispoof tests-core tests-rxtx tests-pmtud tests-ip-fragment \
	tests-firewall tests-nic tests-print tests-queue tests-switch tests-vtep \
	tests-tap tests-thread tests-integration tests-vhost

################################################################################
##                            Tests execution rules                           ##
################################################################################

antispoof : tests-antispoof
	tests/antispoof/test.sh

core : tests-core
	tests/core/test.sh

diode : tests-diode
	tests/diode/test.sh

rxtx : tests-rxtx
	tests/rxtx/test.sh

pmtud : tests-pmtud
	tests/pmtud/test.sh

ip-fragment : tests-ip-fragment
	tests/ip-fragment/test.sh

firewall : tests-firewall
	tests/firewall/test.sh

nic : tests-nic
	tests/nic/test.sh

print : tests-print
	tests/print/test.sh

queue : tests-queue
	tests/queue/test.sh

switch : tests-switch
	tests/switch/test.sh

vtep : tests-vtep
	tests/vtep/test.sh

tap : tests-tap
	tests/tap/test.sh

thread : tests-thread
	tests/thread/test.sh

integration : tests-integration
	tests/integration/test.sh

vhost : tests-vhost
	tests/vhost/test.sh

testclean: testcleanobj
	rm -fv tests-antispoof tests-core tests-diode tests-rxtx tests-pmtud tests-ip-fragment tests-firewall tests-nic tests-print tests-queue tests-switch tests-vtep tests-tap tests-thread tests-integration tests-vhost

testcleanobj:
	rm -fv $(tests_antispoof_OBJECTS) $(tests_core_OBJECTS) $(tests_diode_OBJECTS) $(tests_rxtx_OBJECTS) $(tests_pmtud_OBJECTS) $(tests_ip-fragment_OBJECTS) $(tests_firewall_OBJECTS) $(tests_nic_OBJECTS) $(tests_print_OBJECTS) $(tests_queue_OBJECTS) $(tests_switch_OBJECTS) $(tests_vtep_OBJECTS) $(tests_tap_OBJECTS) $(tests_thread_OBJECTS) $(tests_integration_OBJECTS) $(tests_vhost_OBJECTS)
