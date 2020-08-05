#!/bin/sh
sudo ./bench-vtep -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
