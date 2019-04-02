#!/bin/bash
#RUN TESTS BENCH

cd $TRAVIS_BUILD_DIR
git config user.name "$(GH_USER_NAME)"
git config user.email "$(USER_MAIL)"
export LD="llvm-link"
./configure -p --with-benchmarks CC=clang

set -x
set -e

make
make bench_compile

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
./tests/ip-fragment/bench.sh
