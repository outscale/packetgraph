EXAMPLE_INCLUDED=true

.PHONY: 

################################################################################
##                                    Examples                                ##
################################################################################

example_firewall_SOURCES = examples/firewall/firewall.c
example_firewall_OBJECTS = $(example_firewall_SOURCES:.c=.o)

example_switch_SOURCES = examples/switch/switch.c
example_switch_OBJECTS = examples/switch/switch.o

example_rxtx_SOURCES = examples/rxtx/rxtx.c
example_rxtx_OBJECTS = examples/rxtx/rxtx.o

example_nic_SOURCES = examples/nic/nic.c
example_nic_OBJECTS = examples/nic/nic.o

example_dperf_SOURCES = examples/dperf/dperf.c
example_dperf_OBJECTS = examples/dperf/dperf.o
example_dperf_CFLAGS = $(PG_CFLAGS) -Wno-address-of-packed-member

example_CFLAGS = $(PG_CFLAGS)
example_HEADERS = $(PG_HEADERS)
example_LDFLAGS = -lpacketgraph $(PG_LIBADD) -L$(srcdir) -ldl -lm -lnuma -lpthread -lpcap -lz

example: all example-firewall example-switch example-rxtx example-nic example-dperf

example-firewall: $(example_firewall_OBJECTS)
	$(CC) $(example_CFLAGS) $(example_HEADERS) $(example_firewall_OBJECTS) $(example_LDFLAGS) -o $@
$(example_firewall_OBJECTS): %.o : %.c
	$(CC) -c $(example_CFLAGS) $(example_HEADERS) $< -o $@

example-switch: $(example_switch_OBJECTS)
	$(CC) $(example_CFLAGS) $(example_HEADERS) $(example_switch_OBJECTS) $(example_LDFLAGS) -o $@
$(example_switch_OBJECTS): %.o : %.c
	$(CC) -c $(example_CFLAGS) $(example_HEADERS) $< -o $@

example-rxtx: $(example_rxtx_OBJECTS)
	$(CC) $(example_CFLAGS) $(example_HEADERS) $(example_rxtx_OBJECTS) $(example_LDFLAGS) -o $@
$(example_rxtx_OBJECTS): %.o : %.c
	$(CC) -c $(example_CFLAGS) $(example_HEADERS) $< -o $@

example-nic: $(example_nic_OBJECTS)
	$(CC) $(example_CFLAGS) $(example_HEADERS) $(example_nic_OBJECTS) $(example_LDFLAGS) -o $@
$(example_nic_OBJECTS): %.o : %.c
	$(CC) -c $(example_CFLAGS) $(example_HEADERS) $< -o $@

example-dperf: $(example_dperf_OBJECTS)
	$(CC) $(example_dperf_CFLAGS) $(example_HEADERS) $(example_dperf_OBJECTS) $(example_LDFLAGS) -o $@
$(example_dperf_OBJECTS): %.o : %.c
	$(CC) -c $(example_dperf_CFLAGS) $(example_HEADERS) $< -o $@

exampleclean:
	rm -fv $(example_firewall_OBJECTS) $(example_switch_OBJECTS) $(example_rxtx_OBJECTS) $(example_nic_OBJECTS) $(example_dperf_OBJECTS)

examplefclean: exampleclean
	rm -fv example-firewall example-switch example-rxtx example-nic example-dperf
