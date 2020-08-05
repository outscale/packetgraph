#!/bin/sh
sudo ./bench-diode -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
