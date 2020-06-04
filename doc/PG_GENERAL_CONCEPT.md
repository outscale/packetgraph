# Packetgraph's general concept
## Introduction
Outscale's packetgraph is a solution to link (In a network) virtual machines with some others and/or the real world.<br>
It aims at doing it fast, and for this purpose we will use [DPDK](https://www.dpdk.org/) aka Data Plane Development Kit.<br>
The core idea is to do not "move" packets which cost a lot of memmory and time so we alocate them one time for all.<br>
.<br>


```

 The outer     +---Host Machine---------------------------------------------------------------------+
  World        |                                                                                    |
               |   +--The GRAPH--------------------------------------+                              |
               |   |                                                 |                              |
               |   |                                             +-------+              +---------+ |
               |   |               +---------------------------->| VHOST |<------------>|  VM     | |
               |   |               |                             +-------+              +---------+ |
               |   |               v                                 |                              |
           +---------+        +---------+                        +-------+              +---------+ |
 <-------->|   NIC   |<------>|Switch   |<---------------------->| VHOST |<------------>|  VM     | |
           +---------+        +---------+                        +-------+              +---------+ |
               |   |               ^                                 |                              |
               |   |               |      +--------------+       +-------+              +---------+ |
               |   |               +----->| Firewall     |<----->| VHOST |<------------>|  VM     | |
               |   |                      +--------------+       +-------+              +---------+ |
               |   |                                                 |                              |
               |   +--------------------------------------------------                              |
               |                                                                                    |
               +------------------------------------------------------------------------------------+
```
