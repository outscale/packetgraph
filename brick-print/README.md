# Packetgraph's print brick

This packetgraph's brick is used to print packets informations flowing through
it.

# Building

Print brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build print brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-print into an rpm, simply run ```make package```.

# Usage

Check packetgraph/print.h for documentation.
