# Packetgraph's antispoof

This packetgraph's brick permits to protect a specific side from spoofing
attacks.

## MAC anti-spoofing

All packets comming from the antispoof side must have the specified MAC.
Otherwise, packets are dropped.

## ARP anti-spoofing

When enabled through pg_antispoof_arp_enable(); this drop all packets
comming from the antispoof side which don't respect a pre-built ARP packet.
The current implementation just check that Sender MAC address and
Sender IP address fields respect the specified ones.
Note that only this only allow ARP packets with Hardware type set to
Ethernet (1) and Protocol type set to IP (0x0800).

## Reverse ARP

For the moment, RARP protocol is dropped.

# Building

Antispoof brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build antispoof brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-antispoof into an rpm, simply run ```make package```.

# Usage

Check packetgraph/antispoof.h for documentation.
