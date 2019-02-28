#!/bin/sh
sudo ./bench-tap -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
