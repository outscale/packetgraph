# VHOST Brick

## Introduction

The VHOST brick is the brick used to make the graph communicate with VMs.<br>
The problem while communicating with VMs via "standard way" is that it's really slow.<br>
So here we use the virtio protocol implemented as vhost in DPDK.<br>


## VHOST overview
 ```
+---------------------------+
|                           |
|     +-------------+       |                     +-------------+
|     | Graph's side|       |                     | Host's side |
|     +-------------+       |                     +-------------+
|                           |
|                       |   |
|                       | +--------+              +-------------+             +---------------+
|             edge  <---|-| VHOST  |<------------>| UNIX SOCKET |<----------->|Virtual-Machine|
|                       | +--------+              +-------------+             +---------------+
|                       |   |  ^                                                       ^
|                      Side |  |                                                       |
| ^ ^ ^ ^ ^ ^ ^ ^           |  |                                                       |
+-|-|-|-|-|-|-|-|-----------+  |                                                       |
+-|-|-|-|-|-|-|-|--------------|-------------------------------------------------------|------+
| v v v v v v v v              v                                                       v      |
|                      Host's shared memmory, aka hugepage, containing packets.               |
|                                                                                             |
+---------------------------------------------------------------------------------------------+
```
As previously described, VHOST use an unix socket and a hugepage to communicate via ip.<br>
It manages a queue and reduce memmory write/free operatons.<br>
It's based on a cient(s)/server model, meaning that one server can handle multiple connections through the socket.<br> Only packet address in the hugepage are flowing through the socket.<br>

## Current VHOST brick's status

Currently the VHOST brick only works in SERVER mode... Which means that if packetgraph crash, we will need to reboot VMs...<br>Not a good thing!
