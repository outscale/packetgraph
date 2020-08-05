#!/bin/sh
sudo ./bench-antispoof -c1 -n1 --socket-mem 256 --no-shconf -- "$@"
