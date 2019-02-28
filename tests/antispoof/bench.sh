#!/bin/sh
sudo ./bench-antispoof -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
