Butterfly
=========

Butterfly permits to connect Virtual Machine (VM) NICs and manage their
network flow.

Butterfly permits to isolate, connect and filter traffic between virtual
machines. This is particulary usefull in cloud environments in order to manage
network interactions between Virtual Machines or external network.

# Architecture

Here is the butterfly architecture, all it's software architecture is
organized around connected bricks.

![Butterfly architecture](http://i.imgur.com/zQRXbTm.png)

1. Butterfly use [Data Plane Development Kit (DPDK)](http://dpdk.org/)
to accelerate traffic latency and minimize Operating System impact in packet
filtering.

2. Butterfly use [Virtual Extensible LAN (VXLAN)](http://en.wikipedia.org/wiki/Virtual_Extensible_LAN/)
permetting to isolate traffic between virtual machines over an external
network.

3. Butterfly use a home-made layer 2 switch

4. Firewalling permits to filter network traffic from each virtual machines.
It is based on [NPF](http://www.netbsd.org/~rmind/npf/)

5. Butterfly can manage several Virtual Machines on the same host by using
vhost-user with Qemu.

# Butterfly API

Butterfly offers a [protobuf](https://github.com/google/protobuf/ "Google's protobuf")
defined API to configure all it's parameters.

# What do I need to use Butterfly ?

- A DPDK compatible NIC
- A x86_64 host
- At least Qemu 2.1

# Build instructions

## Build under Debian x86_64

Here are some explainations on how to build Butterfly on Debian SID
(February 2015).

Install needed packages:

    sudo apt-get install git cmake gcc g++ libc6-dev libc6-dbg libc6-i386 libglib2.0-dev libprotobuf-c1 protobuf-c-compiler cppcheck

Let's clone and init submodule:

    git clone git@github.com:benoit-canet/butterfly.git
    cd butterfly
    git submodule init
    git submodule update

And let's build !

    cd butterfly
    cmake .
    make

Now you can get a coffee, it will take some time.

## Other platforms

Feel free to share you build instructions :)

# Question ? Contact us !

Butterfly is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly

