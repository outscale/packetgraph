# Packetgraph ?

Packetgraph is a library aiming to give the user a tool to build networks graph easily, It's built upon the fast [DPDK library](http://dpdk.org/).

[![Build Status](https://travis-ci.org/outscale/packetgraph.svg?branch=master)](https://travis-ci.org/outscale/packetgraph)

The goal of this library is to provide a really EASY interface to
build you own DPDK based application using [Network Function
Virtualization](https://en.wikipedia.org/wiki/Network_function_virtualization)
Everyone is free to use this library to build up there own network application.

Once you have created and connected all bricks in you network graph,
some bricks will be able to poll a burst of packets (max 64 packets)
and let the burst propagate in you graph.

Connections between bricks don't store any packets and each burst will
propagate in the graph without any copy.

Each graph run on one core but you can connect different graph using
Queue bricks (which are thread safe). For example, a graph can be
split on demand to be run on different core or even merged.

If you want a graphical representation of a graph, you can generate a [dot](https://en.wikipedia.org/wiki/DOT_%28graph_description_language%29) output.

![packetgraph features](https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/packetgraph_features.svg "packetgraph features")

# Available bricks (ipv4/ipv6):

- switch: a layer 2 switch
- rxtx: setup your own callbacks to get and sent packets
- tap: classic kernel virtual interface
- vhost: allow to connect a vhost NIC to a virtual machine (virtio based)
- firewall: allow to filter traffic passing through it (based on [NPF](https://github.com/rmind/npf))
- diode: only let packets pass in one direction
- hub: act as a hub device, passing packets to all connected bricks
- nic: allow to pass packets to a NIC of the system (accelerated by DPDK)
- print: a basic print brick to show packets flowing through it
- antispoof(ipv4 only): a basic mac and arp anti-spoofing brick
- vtep: VXLAN Virtual Terminal End Point switching packets on virtual LANs
- queue: temporally store packets between graph
- pmtud(ipv4 only): Path MTU Discovery is an implementation of [RFC 1191](https://tools.ietf.org/html/rfc1191)

A lot of other bricks can be created, check our [wall](https://github.com/outscale/packetgraph/issues?q=is%3Aopen+is%3Aissue+label%3Awall) ;)

# How should I use packetgraph ?

![packetgraph usage flow](https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/packetgraph_flow.svg "packetgraph usage flow")

# Examples

To build and run examples, you may first check how to build packetgraph below and adjust your configure command before make:
```
$ ./configure --with-examples
$ make
```

To run a specific example, check run scripts in tests directories:
```
$ ./examples/switch/run_vhost.sh
$ ./examples/switch/run.sh
$ ./examples/firewall/run.sh
$ ./examples/rxtx/run.sh
...
```

# Building

You will need to build DPDK before building packetgraph.

## Install needed tools

You may adapt this depending of your linux distribution:
```
$ sudo apt-get install automake libtool libpcap-dev libglib2.0-dev libjemalloc-dev
```

## Build DPDK

```
$ git clone http://dpdk.org/git/dpdk
$ cd dpdk
$ git checkout -b v16.07 v16.07
$ make config T=x86_64-native-linuxapp-gcc
```
Edit build/.config and be sure to set the following parameters to 'y':
- CONFIG_RTE_LIBRTE_PMD_VHOST
- CONFIG_RTE_LIBRTE_VHOST
- CONFIG_RTE_LIBRTE_PMD_PCAP
```
$ make EXTRA_CFLAGS='-fPIC'
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

# Contribute

New to packetgraph ? Want to contribute and/or create a new brick ?
Some [developper guidelines](docs/contrib.md) are available.

# Question ? Contact us !

Packetgraph is an open-source project, feel free to [chat with us on IRC](https://webchat.freenode.net/?channels=betterfly&nick=packetgraph_user)

> server: irc.freenode.org

> chan: #betterfly

