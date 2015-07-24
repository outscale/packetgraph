/!\ WIP ALERT /!\

This reprository is under active developpement and is subject to heavy
modifications.

# Packetgraph

Packetgraph library is build upon DPDK making a collection of network bricks
you can connect to form a network graph.
The goal of this project is to provides a low-latency software defined
network.
Everyone is free to use this library to build up there own network
infrastructure.

Here are current developped bricks available in packetgraph:

- switch: a layer 2 switch
- vhost: allow to connect a vhost NIC to a virtual machine (virtio based)
- firewall: allow to filter traffic passing through it (based on NPF)
- diode: only let packets pass in one direction
- hub: act as a hub device, passing packets to all connected bricks
- nic: allow to pass packets to a NIC of the system (accelerated by DPDK)

Comming soon:

- vtep: VXLAN Virtual Terminal End Point switching packets on virtual LANs
- antispoof: a basic arp anti-spoofing brick

Futur bricks:

- bond: permits to bond two physical nic
- ipsec: for authenticating and encrypting IP packet of a communication session
- gre: generic routing encapsulationn, a tunneling protocol
- vpn: virtual private network, a tunneling protocol
- nat: manage network address translation
- dhcp: add a dhcp server (Dynamic Host Configuration Protocol)

# Building

In order to reduce dependencies, the project has been splitted in several libs:
- A "core" library on which all bricks relies on.
- Each brick is a library.

Check each libraries to build them :)

# Question ? Contact us !

Packetgraph is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly
