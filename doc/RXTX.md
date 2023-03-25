# RXTX Brick

## Introduction

he RXTX brick is a monopole single-edge brick.<br>
It is intended mainly for testing and benchmarking purpose.<br>
We use it to create packet and send them through a graph and/or receive them.<br>

## Usage

Here is the constructor:
```
struct pg_brick *pg_rxtx_new(const char *name,
                             pg_rxtx_rx_callback_t rx,
                             pg_rxtx_tx_callback_t tx,
                             void *private_data)
```
As we can see, we give it two callbacks as parameters:

* `pg_rxtx_rx_callback_t rx`: The method that will be used to send packets.
* `pg_rxtx_rx_callback_t tx`: The method that will be called whene the brick receive packets.
