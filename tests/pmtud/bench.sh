#!/bin/sh
sudo ./bench-pmtud -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
