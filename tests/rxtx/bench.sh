#!/bin/sh
sudo ./bench-rxtx -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
