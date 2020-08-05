#!/bin/sh
sudo ./tests-nic -c1 -n1 --socket-mem 256 --no-shconf --vdev=eth_ring0
