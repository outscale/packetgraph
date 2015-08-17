# Packetgraph's diode

This packetgraph's brick is used to let packets pass in one way only.
This is usefull to make sure that a part of a graph will never send packets
to the other side.

# Building

Diode brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build diode brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-diode into an rpm, simply run ```make package```.

# Usage

A diode is pretty simple to use: it only need a direction where packets can
flow.

A real life example would be to plug a diode on a hub to mirror all traffic
to a vhost for sniffing or logging purposes.

```
[nic]---[hub]-----------[nic]
          |
          +--[>diode>]--[nic]
```
```
struct brick *d;
d = diode_new("my diode", 1, 1, EAST_SIDE, &error);
```

In this example, packets who comes from the west side to the east side will
be able to go through the diode. Packets coming from the west side will be
ignored.

Check packetgraph/diode.h for documentation of diode_new().
