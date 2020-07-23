#!/bin/bash

set -x
set -e

make style
make
make tests_compile
make bench_compile

if [ "$(whoami)" == "travis" ]; then
    if [ ! -e "/usr/bin/doxygen" ]; then
	sudo curl https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/doxygen.sh -o /usr/bin/doxygen
	sudo chmod +x /usr/bin/doxygen
    fi
    make doc
fi

./tests/accumulator/test.sh
./tests/antispoof/test.sh
./tests/core/test.sh
./tests/diode/test.sh
./tests/firewall/test.sh
./tests/nic/test.sh
./tests/print/test.sh
./tests/queue/test.sh
./tests/switch/test.sh
./tests/vtep/test.sh
./tests/rxtx/test.sh
./tests/pmtud/test.sh
./tests/tap/test.sh
./tests/thread/test.sh
./tests/udp-filter/test.sh

./tests/accumulator/bench.sh
./tests/antispoof/bench.sh
./tests/core/bench.sh
./tests/diode/bench.sh
./tests/firewall/bench.sh
./tests/nic/bench.sh
./tests/print/bench.sh
./tests/queue/bench.sh
./tests/switch/bench.sh
./tests/vtep/bench.sh
./tests/rxtx/bench.sh
./tests/pmtud/bench.sh
./tests/tap/bench.sh

if [ "$(whoami)" != "travis" ]; then
    ./tests/integration/test.sh
    ./tests/vhost/test.sh
fi

make cov
