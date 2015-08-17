# Packetgraph's vhost

This brick is quite speciale as it permits to connect to a vhost NIC using
virtio. This is particulary usefull to connect virtual machines NICs to
packetgraph.

This brick need a path were are stored unix sockets (/tmp for example).
For the moment, this path is global and not specific to a brick instance.

# Building

Vhost brick needs:

- dpdk built
- packetgraph-core installed OR ...
- ```export PG_CORE=/abs/path/to/core/build/dir```

Just create a build folder and build vhost brick:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

For details on how to build dpdk, check packetgraph-core's documentation.

to package packetgraph-vhost into an rpm, simply run ```make package```.

# Usage

To use this brick you need to first call vhost_start with the directore path
use to store sockets as argument.

To create a new brick, use vhost_new.
Vhost_new use the same arguments as other "new" functions,
with an aditional "output" parameter use to know on which side is the "VM".

If the packets you burst are comming from the WEST_SIDE so the
direction should be EAST_SIDE because the VM is on the EAST, then when call,
brick_poll will automatically send packets to the WEST.

Vhost_new will create and store a socket in the directorry set in
vhost_start with the name of the brick.

So if your socket path is "/tmp" and you socket name "joe" your path will be:
/tmp/joe

You can get the socket path create by your brick with brick_handle_dup,
which will return a newly-allocated char * contening the path to the socket.

Once you have your socket path and your brick you just need to spam your VM
using the socket path from the brick, then you can start polling packets with
brick_poll.
