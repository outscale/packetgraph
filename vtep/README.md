# Packetgraph's vtep

This packetgraph's brick is a vtep as describt in
https://tools.ietf.org/html/rfc7348.

The vtep brick allow VM in the same network, but from to diferent Computer or
Garph to comunicate.

The vtep brick, will encapsulate the packets comming from one side
with a vxlan header, and decapsulate the packets comming from the other side.

This implementation does not support the optional VLAN tag in the VXLAN
header

This implementation expects that the IP checksum will be offloaded to
the NIC

# Building

## Prerequisites

vtep brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```
- packetgraph-brick-hub installed OR ...
- ```export PG_HUB=/abs/path/to/brick-hub/build/dir```

For details on how to build dpdk, check packetgraph-core's documentation.

## Build

Just create a build folder and build firewall:

```
$ export RTE_SDK=/you/path/to/dpdk
$ mkdir build
$ cd build
$ cmake ..
$ make
```

to package packetgraph-firewall into an rpm, simply run ```make package```.

# Usage

To create a vtep brick, call pg_vtep_new().

To tell the vtep than a neibough brick correspond to a VNI, you need to:
- Link brick with the vtep
- use pg_vtep_add_vni to register the brick to a VNI

To remove a brick from a VNI, just unkink the connect brick.
Note that you can't add two bricks on the same VNI.

If you want to get the vtep mac, you can call vtep_get_mac().
