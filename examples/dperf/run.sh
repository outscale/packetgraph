#!/bin/sh
sudo ./example-dperf -c1 -n1 --socket-mem 256 --no-shconf "$@"
