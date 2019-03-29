#!/bin/bash

set -x
set -e

make fclean
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
	bundle exec rake test:./tests/antispoof/test.sh
	bundle exec rake test:./tests/core/test.sh
	bundle exec rake test:./tests/diode/test.sh
	bundle exec rake test:./tests/firewall/test.sh
	bundle exec rake test:./tests/nic/test.sh
	bundle exec rake test:./tests/print/test.sh
	bundle exec rake test:./tests/queue/test.sh
	bundle exec rake test:./tests/switch/test.sh
	bundle exec rake test:./tests/vtep/test.sh
	bundle exec rake test:./tests/rxtx/test.sh
	bundle exec rake test:./tests/pmtud/test.sh
	bundle exec rake test:./tests/tap/test.sh
	bundle exec rake test:./tests/ip-fragment/test.sh
	bundle exec rake test:./tests/thread/test.sh
	
	bundle exec rake test:./tests/antispoof/bench.sh
	bundle exec rake test:./tests/core/bench.sh
	bundle exec rake test:./tests/diode/bench.sh
	bundle exec rake test:./tests/firewall/bench.sh
	bundle exec rake test:./tests/nic/bench.sh
	bundle exec rake test:./tests/print/bench.sh
	bundle exec rake test:./tests/queue/bench.sh
	bundle exec rake test:./tests/switch/bench.sh
	bundle exec rake test:./tests/vtep/bench.sh
	bundle exec rake test:./tests/rxtx/bench.sh
	bundle exec rake test:./tests/pmtud/bench.sh
	bundle exec rake test:./tests/tap/bench.sh
	bundle exec rake test:./tests/ip-fragment/bench.sh
else
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
    ./tests/integration/test.sh
    ./tests/vhost/test.sh
fi
