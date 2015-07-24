# Packetgraph's hub

This packetgraph's brick is used to create a simple hub dispatching packets
to all connected bricks (except the one where the packet come from).

# Building

Diode brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build hub brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-hub into an rpm, simply run ```make package```.

# Usage

A hub is pretty simple to use and do not need any configuration except it's
number of ports on each sides (like all bricks).

Here is a hub of 16 ports (8 on both sides):
```
struct brick *h;
h = hub_new ("my hub", 8, 8, &error);
```
