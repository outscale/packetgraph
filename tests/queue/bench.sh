#!/bin/sh
sudo ./bench-queue -c1 -n1 --socket-mem 64 --no-shconf -- "$@"
