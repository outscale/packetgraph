# Dperf example

dperf uses rxtx rxtx brick to generate packets (useful for testing).

```
[nic]---[rxtx]
```

# Build

Just add `--with-examples` to your ./configure options and run make.
You can run example with a simple script:
```
./examples/dperf/run.sh
```

## Using non-DPDK NICs

We can also use NICs which are not optimized to be used with DPDK.
This will just requiere to pass some additional DPDK arguments to our example:

```
$ ./examples/dperf/run.sh --vdev=eth_pcap0,iface=eth0
```

