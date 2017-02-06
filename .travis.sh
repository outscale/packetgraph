#!/bin/bash
set -x
set -e

make

make style

make tests-antispoof
make tests-core
make tests-diode
make tests-firewall
make tests-integration
make tests-nic
make tests-print
make tests-queue
make tests-switch
make tests-vhost
make tests-vtep
make tests-rxtx
make tests-pmtud
make tests-tap
make tests-ip-fragment

./../tests/antispoof/test.sh
./../tests/core/test.sh
./../tests/diode/test.sh
./../tests/firewall/test.sh
./../tests/nic/test.sh
./../tests/print/test.sh
./../tests/queue/test.sh
./../tests/switch/test.sh
./../tests/vtep/test.sh
./../tests/rxtx/test.sh
./../tests/pmtud/test.sh
./../tests/tap/test.sh
./../tests/ip-fragment/test.sh

./../tests/antispoof/bench.sh
./../tests/core/bench.sh
./../tests/diode/bench.sh
./../tests/firewall/bench.sh
./../tests/nic/bench.sh
./../tests/print/bench.sh
./../tests/queue/bench.sh
./../tests/switch/bench.sh
./../tests/vtep/bench.sh
./../tests/rxtx/bench.sh
./../tests/pmtud/bench.sh
./../tests/tap/bench.sh
./../tests/ip-fragment/bench.sh
