#!/bin/bash

cd $TRAVIS_BUILD_DIR
git config user.name "$(GH_USER_NAME)"
git config user.email "$(USER_MAIL)"
export LD="llvm-link"
./configure CC=clang -p --with-benchmarks

set -x
set -e

make
make tests_compile
make bench_compile

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
