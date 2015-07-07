WIP ALERT : This reprository is under active developpement and is subject to heavy modification.

Butterfly
=========

Butterfly permits to connect Virtual Machine (VM) NICs and manage their
network flow.

Butterfly permits to isolate, connect and filter traffic between virtual
machines using VXLAN. This is particulary usefull in cloud environments in
order to manage network interactions between Virtual Machines or external
network.

# Architecture

Butterfly has two main components: Packetgraph library and Butterfly.

## Packetgraph

Packetgraph library is build upon DPDK making a collection of network bricks
you can connect them to form a network graph.
Everyone is free to use this library to build up there own network
infrastructure accelerated.

Here are current developped bricks available in packetgraph:

- switch: a layer 2 switch
- vhost: allow to connect a vhost NIC to a virtual machine (virtio based)
- firewall: allow to filter traffic passing through it (based on NPF)
- diode: only let packets pass in one direction
- hub: act as a hub device, passing packets to all connected bricks
- nic: allow to pass packets to a NIC of the system (accelerated by DPDK)
- vtep: VXLAN Virtual Terminal End Point switching packets on virtual lans
- packetgen: a debug brick allowing to generate packets in the graph

Packetgraph is made in C language and a minimal C++ wrapper (packetgraph++) is
also available.

Butterfly is a specific use case of packetgraph but you can navigate in
packetgraph examples to see how to use it.

## Butterfly

Butterfly is built on top of packetgraph++ and build a network graph for
it's own goal: connect virtual machines on different hosts on layer 2
with the best performances.

Here is the butterfly architecture 

![Butterfly architecture](http://i.imgur.com/zQRXbTm.png)

All the bricks use [Data Plane Development Kit (DPDK)](http://dpdk.org/)

The software architecture is organized around the following
connected bricks (from Packetgraph):

1. Butterfly accelerate traffic latency and minimize Operating System
impact in packet filtering. This corresponds to "nic" brick in packetgraph.

2. Butterfly use [Virtual Extensible LAN (VXLAN)]
(http://en.wikipedia.org/wiki/Virtual_Extensible_LAN/)
permetting to isolate traffic between virtual machines over an external
network. Each links from a specific side corresponds to a VXLAN, the other
side corresponds to VTEP endpoint ("vtep" brick in packetgraph).

3. Butterfly use the layer 2 switch from packetgraph

4. Firewalling allow to filter network traffic from each virtual machines.
It is based on [NPF](http://www.netbsd.org/~rmind/npf/). This corresponds to
the "firewall" brick in packetgraph.

5. Butterfly can manage several Virtual Machines on the same host by using
vhost-user with Qemu. This corresponds to "vhost" brick in packetgraph.

# Butterfly API

Butterfly offers a [protobuf](https://github.com/google/protobuf/ "Google's protobuf")
defined API to configure all it's parameters.

# What do I need to use Butterfly ?

- A DPDK compatible NIC
- A x86_64 host
- At least Qemu 2.1

# Build instructions

Here are some explainations on how to build Butterfly
(February 2015).

Install needed packages:

# Debian SID
    apt-get install git cmake gcc g++ libc6-dev libc6-dbg libc6-i386 libglib2.0-dev libprotobuf-c1 protobuf-c-compiler cppcheck qemu

# Arch Linux
    pacman -S git multilib-devel base-devel cmake glib2 protobuf-c cppcheck qemu

Let's clone and init submodule:

    git clone git@github.com:benoit-canet/butterfly.git
    cd butterfly
    git submodule init
    git submodule update

And let's build !

    cd butterfly
    cmake .
    make

Now you can get a coffee, it will take some time (https://xkcd.com/303/).

## CentOS 7


### Install pre-requisite

- yum install git cmake gcc gcc-c++ glib2-devel.x86_64 glib2.x86_64 flex bison bc libstdc++.i686 perl-Data-Dumper.x86_64 libpcap-devel fuse-devel wget zlib-devel

- wget http://ftp.free.fr/mirrors/fedora.redhat.com/fedora/linux/releases/21/Everything/x86_64/os/Packages/u/userspace-rcu-0.8.1-5.fc21.x86_64.rpm
- rpm -i userspace-rcu-0.8.1-5.fc21.x86_64.rpm
- wget http://ftp.free.fr/mirrors/fedora.redhat.com/fedora/linux/releases/21/Everything/x86_64/os/Packages/u/userspace-rcu-devel-0.8.1-5.fc21.x86_64.rpm
- rpm -i userspace-rcu-devel-0.8.1-5.fc21.x86_64.rpm
- wget http://noxt.eu/~rmind/libprop-0.6.5-1.el7.centos.x86_64.rpm
- rpm -i libprop-0.6.5-1.el7.centos.x86_64.rpm

### Get and install patched libcdb

- git clone https://github.com/rmind/libcdb
- cd libcdb
- make rpm
- rpm -ihv RPMS/x86_64/libcdb-*

### Get and install NPF

- git clone https://github.com/rmind/npf
- cd npf/pkg
- make rpm
- rpm -ihv RPMS/x86_64/npf-*

### Get and install Qemu

- git clone git://git.qemu-project.org/qemu.git
- cd qemu
- git submodule update --init pixman
- git submodule update --init dtc
- ./configure
- make
- make install
- sudo -ln -s /usr/local/bin/qemu-system-x86_64 /usr/bin/
(tests will reach qemu here)

### Configure Hugepages for Centos

#### Enable hugepages

- echo "vm.nr_hugepages=550" >> cat /etc/sysctl.conf
- sysctl -p /etc/sysctl.conf

check:
- grep HugePages /proc/meminfo

#### Mount hugepages

- mkdir /mnt/huge
- mount -t hugetlbfs none /mnt/huge

### Build and run tests

- mkdir build
- cd build
- cmake ..
- make
as super user:
- make tests-all

## Other platforms

Feel free to share you build instructions :)

# Question ? Contact us !

Butterfly is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly

