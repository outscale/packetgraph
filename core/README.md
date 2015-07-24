# Packetgraph Core

packetgraph-core is a part of packetgraph project. It implement the core
elements permiting to implement bricks and connect them.

# Building

## CentOS Linux 7

Build prerequisites:

You will need some basic tools and build dpdk (see below).
```yum install git cmake gcc gcc-c++ glib2-devel.x86_64 glib2.x86_64```

## DPDK build

If you don't have DPDK, get it and build it:

```
$ git clone http://dpdk.org/git/dpdk
$ cd dpdk
$ export RTE_SDK=$(pwd)
$ make config T=x86_64-native-linuxapp-gcc CONFIG_RTE_LIBRTE_VHOST=y CONFIG_RTE_LIBRTE_PMD_PCAP=y
$ make
```

## packetgraph-core build:

In 'core' directory:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

To package packetgraph-core into an rpm, simply run ```make package```.

## Packaging

After building:

- yum install rpm-build
- make package

