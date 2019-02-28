#!/bin/sh
sudo ./bench-firewall -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
