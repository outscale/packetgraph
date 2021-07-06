# Packetgraph's brick concept.

## Overview.

Each brick have 2 sides: East and West (except for monopole brick that have only one).<br>
Each side can have 0 or more edges (except for dipole brick that have one edge per side).<br>
Edges are stored in brick's sides and pointing to another brick.<br>
So to create a link we need 2 edges... One from the first one pointing to the second one and vice versa.<br>
Note: the side notion is for the packet's source, because it goes directly to a brick.<br>
<br>
A basic dipole brick shema:<br>
```
               |             |
  edge  0  <---| +---------+ |--->edge  0
  edge ... <---|-|  BRICK  |-|--->edge ...
  edge  n  <---| +---------+ |--->edge  n
               |             |
               |             |
               |             |
               |             |
           WEST SIDE     EAST SIDE
```
And now 2 basic bricks linked together:<br>
```
                      +-----B-West to A----+   +---A-East to B-------+
                      |                    |   |                     |
               |      V      |             |   |              |      V      | 
  edge  0  <---| +---------+ |--->edge 0-------+  edge  0 <---| +---------+ |--->edge  0
  edge ... <---|-| BRICK A |-|--->edge ... |      edge ...<---|-| BRICK B |-|--->edge ...
  edge  n  <---| +---------+ |--->edge n   +------edge  n <---| +---------+ |--->edge  n
               |             |                                |             |
               |             |                                |             |
               |             |                                |             |
               |             |                                |             |
           WEST SIDE     EAST SIDE                        WEST SIDE     EAST SIDE
```
<br>
Why having sides?<br>
Because it makes it easier to perform operations between two sides such as acting as a diode, filter...<br>

### Warning!

While creating links, make sure that there is not 2 bricks modifying packets (VXLAN, VTEP) on the same side!<br>
Here is why:<br>
To improve our perfs, we do not copy packets so if a brick modify them, they will be modified for all other bricks on this side.<br>
<br>
Example:<br>
We want to link some VMs to a VTEP. So we need VHOST bricks for each VMs and a switch.<br>
The VTEP must NOT be on the same side than VHOST bricks.<br>
Sides are decided by the order of the arguments of the method `pg_brick_link(BRICK_A, BRICK_B)`.<br>
So basically we will do so:

* `pg_brick_link(SWITCH, VTEP);`
* `pg_brick_link(VHOST_0, SWITCH);`
* `pg_brick_link(VHOST_1, SWITCH);`
* `pg_brick_link(VHOST_2, SWITCH);`

So we are sure that VTEP and VHOST_n are not on the same side.<br>
If we cannot isolate as we want the VTEP, a NOT recommended way would be to disable the `NOCOPY` flag.
 
## How monopole/single edge brick works:
### Single edge:
As the following content shows it, `edge` and `edges` are in an `union` so basically one side can have `edge` OR `edges`.
```
struct pg_brick_side {
	[...]
	/* Edges is use by multipoles bricks,
	 * and edge by dipole and monopole bricks
	 */
	union {
		struct pg_brick_edge *edges;	/* edges */
		struct pg_brick_edge edge;
	};
};
```
### Single side:
As the following content shows it, `side` and `sides` are in an `union` so basically one side can have `side` OR `sides`.
```
struct pg_brick {
        [...]
	union {
		struct pg_brick_side sides[PG_MAX_SIDE];
		struct pg_brick_side side;
	};
};
```

## Brick's common packet operations.

Packets are going through bricks via bursts. Bursts are started only from Inputs/Outputs of the graph (IO bricks: VHOST, NIC, RXTX, VTEP) during a graph poll. so polling bricks that are not IO bricks is a nonsense.<br>

```
struct pg_brick {
  [...]
	/* Accept a packet burst */
	int (*burst)(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask, struct pg_error **errp);
	/* polling */
	int (*poll)(struct pg_brick *brick,
		    uint16_t *count, struct pg_error **errp);
  [...]
};
```

According to the `pg_brick` strcture, we have two methods dealing with packets:

* `int burst([...]);`<br>
  Called to burst packets through an edge through a side to another brick though her other side.<br>
  Example: If I burst from brick A through East side and edge 1 (pointing to brick B),<br>  brick B will receive it though West side.<br>
  Bursts are not moving packets! Bursts are passing their adress in the hugepage to another brick's burst (Each burst call the next's brick burst until going outside the graph via an IO brick).
* `int poll([...]);`<br>
  Called during a graph poll for an input brick. Most of the time it will burst packets received/created through the graph.


