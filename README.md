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

## Other platforms

Feel free to share you build instructions :)

# Question ? Contact us !

Butterfly is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly

