# Packetgraph's nic

This packetgraph's brick is used to connect a physical nic to packetgraph.
nic parameters are passed to the application using program's arguments.
You can pass a dpdk compatible NIC or just any system NIC.

# Building

Nic brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build nic brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-nic into an rpm, simply run ```make package```.

# Usage


To use this brick you need to first call nic_start, this function will initialisse the
dpdk driver's

To create a new brick, use nic_new.
nic_new use the same arguments as other "new" functions,
with an aditional "output" parameter use to know on which side is the nic.

If the packets you burst are comming from the WEST_SIDE so the
direction should be EAST_SIDE because the nic is on the EAST, then when call,
brick_poll will automatically send packets to the WEST.

To poll you packets, you just need to call brick_poll, which will transmit
the newly-polled packet to his neibour.

When you burst packets on this brick, the brick will burst_tx on the nic.