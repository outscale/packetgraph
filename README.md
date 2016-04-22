# Packetgraph

Packetgraph library is build upon [DPDK](http://dpdk.org/) making a collection
of network bricks you can connect to form a network graph.
The goal of this project is to provide a low-latency software defined
network.
Everyone is free to use this library to build up there own network
infrastructure.

Here are current developped bricks available in packetgraph:

- switch: a layer 2 switch
- vhost: allow to connect a vhost NIC to a virtual machine (virtio based)
- firewall: allow to filter traffic passing through it (based on [NPF](https://github.com/rmind/npf))
- diode: only let packets pass in one direction
- hub: act as a hub device, passing packets to all connected bricks
- nic: allow to pass packets to a NIC of the system (accelerated by DPDK)
- print: a basic print brick to show packets flowing through it
- antispoof: a basic mac and arp anti-spoofing brick
- vtep: VXLAN Virtual Terminal End Point switching packets on virtual LANs
- queue: temporally store packets between graph

# Examples

Here are some possible graphics you can do with paquetgraph. You will find
some implementations in "examples" folder.
Ofcourse, they are just more or less complex examples and you can program your own architecture to be dynamic.

A simple firewall betweek two NIC:

![A simple firewall](http://i.imgur.com/suqQAbG.png "A simple firewall between two NICS")

A network switch:

![A switch with several nics around](http://i.imgur.com/GT60CpA.png "A switch with several nics around")

Several virtual machines connected to NIC through a switch:

![Virtual machines bridged with the NIC](http://i.imgur.com/UnDYTLB.png "Virtual machines bridged with the NIC")

Connecting virtual machines through VXLAN network:

![Virtual machines connected to a vxlan and protected by a firewall](http://i.imgur.com/Mnxid6n.png "Virtual machines connected to a vxlan and protected by a firewall")

# Building

You will need to build DPDK and install NPF before building packetgraph.

## Build DPDK

```
$ git clone http://dpdk.org/git/dpdk
$ cd dpdk
$ git checkout -b v16.04 v16.04
$ make config T=x86_64-native-linuxapp-gcc
```
Edit build/.config and be sure to set the following parameters to 'y':
- CONFIG_RTE_LIBRTE_PMD_VHOST
- CONFIG_RTE_LIBRTE_VHOST
- CONFIG_RTE_LIBRTE_PMD_PCAP
```
$ make EXTRA_CFLAGS='-fPIC'
```

## Install NPF

Here are some details to build npf and package it (thoses instructions has been made using CentOS 7).

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

## Build packetgraph
```
$ git clone https://github.com/outscale/packetgraph.git
$ cd packetgraph
$ ./autogen.sh
$ ./configure RTE_SDK=/path/to/you/dpdk/folder
$ make
$ make install
```

# License

Packetgraph project is published under [GNU GPLv3](http://www.gnu.org/licenses/quick-guide-gplv3.en.html).
For more information, check LICENSE file.

# Question ? Contact us !

Packetgraph is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly

