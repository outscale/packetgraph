# Packetgraph's switch

This packetgraph's brick is used to simulate a layer 2 switch.

# Building

Switch brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build switch brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-switch into an rpm, simply run ```make package```.

# Usage

A switch is pretty simple to use and do not need any configuration except it's
number of ports on each sides (like all bricks).

Here is a switch of 16 ports (8 on both sides):
```
struct brick *h;
h = switch_new ("my switch", 8, 8, &error);
```
