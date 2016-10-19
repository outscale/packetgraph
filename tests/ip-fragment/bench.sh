#!/bin/sh
sudo ./bench-ip-fragment -c1 -n1 --socket-mem 124 --no-shconf -- $@
