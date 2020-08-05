#!/bin/sh
sudo ./bench-accumulator -c1 -n1 --socket-mem 256 --no-shconf -- $@
