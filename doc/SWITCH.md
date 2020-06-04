# SWITCH Brick

## Introduction

The switch brick is a brick doing what does a switch do in "real life" as described [here](https://en.wikipedia.org/wiki/Network_switch).<br>
The core feature being that when we receive an incoming packet, trying to reach a mac address, there's two cases:

* We know on which interface we can find it and so we forward it directly through the right interface..

* We don't know where is the packet's destination so we broadcast it on all interfaces. Then, once we get the answer, we update the MAC TABLE linking MAC address with INTERFACES. So the next time we will know where to forward the packet.

Another thing is the forget feature: if a line in the mac table hasn't been used since a long time, we forget it!<br>
However it is not working yet!

## Basic example: connect 3 VMs to a NIC.

```

 The outer     +---Host Machine---------------------------------------------------------------------+
  World        |                                                                                    |
               |   +--The GRAPH--------------------------------------+                              |
               |   |                                                 |                              |
               |   |                                             +-------+              +---------+ |
               |   |      |                 |<------------------>| VHOST |<------------>|  VM     | |
               |   |      |                 |                    +-------+              +---------+ |
               |   |      |                 |                        |                              |
           +---------+    |   +---------+   |                    +-------+              +---------+ |
 <-------->|   NIC   |<-->|---|Switch   |---|<------------------>| VHOST |<------------>|  VM     | |
           +---------+    |   +---------+   |                    +-------+              +---------+ |
               |   |      |                 |                        |                              |
               |   |      |                 |                    +-------+              +---------+ |
               |   |      |                 |<------------------>| VHOST |<------------>|  VM     | |
               |   |  WEST SIDE         EAST SIDE                +-------+              +---------+ |
               |   |                                                 |                              |
               |   +--------------------------------------------------                              |
               |                                                                                    |
               +------------------------------------------------------------------------------------+
```

Note: it's always a good practice to link to one side all "subnet" devices and to another the "upper" device (No matter if it's EAST or WEST!).<br>
Here is the reason:<br>
Basically, if we heve a brick modifying packets such as VTEP or VXLAN, we should isolate it on a side. In fact, to manage packets faster, we do not copy them so be careful to do not modify them! They would be modified for all bricks on this side.<br>
Please refer to the [warning section of the brick concept's overview](BRICK_CONCEPT.md) for more informations.

## Let's go deeper into the MAC TABLE.

wip
