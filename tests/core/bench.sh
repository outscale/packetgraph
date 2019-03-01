#!/bin/sh
sudo ./bench-core -c1 -n1 --socket-mem 64 --no-shconf -- $@
