# Packetgraph's firewall

This packetgraph's brick is firewall IP.
It uses [NPF](http://www.netbsd.org/~rmind/npf/) firewall which has been
especially patched by Mindaugas to be compatible with DPDK.

The firewall can be used to inject rules in a pcap-filter expression and
can be statefull if desired.
Note that:
- It only filter ip packets (non-ip packets will pass).
- It default policy is set to block.

Real life example (which is available in examples folder):

```
[nic]--[firewall]--[nic]
```

# Building

## Prerequisites

Firewall brick needs:

- dpdk built
- npf installed
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

For details on how to build dpdk, check packetgraph-core's documentation.

### NPF build details

Here are some details to build npf and package it (thoses instructions
has been made using CentOS 7).

Get and install jemalloc:
```
yum install jemalloc
```

Get and install Libcdb:
```
$ git clone https://github.com/rmind/libcdb
$ cd libcd
$ make rpm
$ rpm -ihv RPMS/x86_64/libcdb-*
```

Install proplib RPM package (which contains a patch):
```
$ wget http://noxt.eu/~rmind/libprop-0.6.5-1.el7.centos.x86_64.rpm
$ rpm -ihv libprop-0.6.5-1.el7.centos.x86_64.rpm
```

Install libqsbr:
```
$ git clone https://github.com/rmind/libqsbr.git
$ cd libqsbr/pkg
$ make rpm
$ rpm -ihv RPMS/x86_64/*.rpm
```

Finally, get the standalone version of npf and build it:
```
$ git clone https://github.com/rmind/npf.git
$ cd npf/pkg
$ make clean
$ make npf
$ rpm -hvi RPMS/x86_64/npf-*
```

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

To create an instance of a firewall brick, just call firewall_new(). At this
point, your firewall block all IP traffic (and IP only).

You can now add several rules using firewall_rule_add() to add your rules but
them won't be effective once call. You will need to call firewall_reload()
to make the firewall load all rules.

To flush rules, just call firewall_rule_flush(). Like firewall_rule_add(), you
also will need to call firewall_reload() to make this effective.

You can also use firewall_rule_flush() and firewall_rule_add() in any order
before calling firewall_reload().

To see details about firewall functions signatures, check firewall.h.
To see a more pactical way to use this brick, just check the firewall examples.

