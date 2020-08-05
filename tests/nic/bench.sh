#!/bin/sh
sudo ./bench-nic -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
