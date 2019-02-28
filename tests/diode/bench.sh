#!/bin/sh
sudo ./bench-diode -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
