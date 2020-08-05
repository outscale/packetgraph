#!/bin/sh
sudo ./bench-core -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
