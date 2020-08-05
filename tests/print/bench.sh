#!/bin/sh
sudo ./bench-print -c1 -n1 --socket-mem 256 --no-shconf -- "$@" 2> /dev/null
