# RXTX Example

This example shows a simple example of packetgraph application using RXTX brick.
RXTX brick allow user to easily retrieve and send packets from packetgraph.

```
[nic]---[rxtx]
```

# Build

Just add `--with-examples` to your ./configure options and run make.
You can run example with a simple script:
```
./examples/rxtx/run.sh
```

## Using non-DPDK NICs

We can also use NICs which are not optimized to be used with DPDK.
This will just requiere to pass some additional DPDK arguments to our example:

```
$ ./examples/rxtx/run.sh --vdev=eth_pcap0,iface=eth0
```

