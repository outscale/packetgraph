/!\ WIP ALERT /!\

This reprository is under active developpement and is subject to heavy
modifications.

# Packetgraph

Packetgraph library is build upon [DPDK](http://dpdk.org/) making a collection
of network bricks you can connect to form a network graph.
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
- print: a basic print brick to show packets flowing through it

Comming soon:

- vtep: VXLAN Virtual Terminal End Point switching packets on virtual LANs
- antispoof: a basic arp anti-spoofing brick

# Examples

Here are some possible graphics you can do with paquetgraph. You will find
some implementations in "examples" folder.

![A simple firewall](http://i.imgur.com/suqQAbG.png "A simple firewall")

![A switch with several nics around](http://i.imgur.com/GT60CpA.png "A switch with several nics around")

![Virtual machines bridged with the NIC](http://i.imgur.com/UnDYTLB.png "Virtual machines bridged with the NIC")

![Virtual machines connected to a vxlan and protected by a firewall](http://i.imgur.com/Mnxid6n.png "Virtual machines connected to a vxlan and protected by a firewall")

# Building

In order to reduce dependencies, the project has been splitted in several libs:
- A "core" library on which all bricks relies on.
- Each brick is a library.

Check each libraries to build them :)

# Question ? Contact us !

Packetgraph is an open-source project, feel free to contact us on IRC:

> server: irc.freenode.org

> chan: #betterfly
