#!/bin/bash

cd $TRAVIS_BUILD_DIR
git config user.name "$(GH_USER_NAME)"
git config user.email "$(USER_MAIL)"
export LD="llvm-link"
./configure -p CC=clang

set -x
set -e

make
make tests_compile

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
./tests/ip-fragment/test.sh
./tests/thread/test.sh
