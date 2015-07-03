#!/bin/sh

# Butterfly root
BUTTERFLY_ROOT=$1
BUTTERFLY_BUILD_ROOT=.
TEST_DIR=$BUTTERFLY_ROOT/packetgraph/tests/

# Test Butterfly build root
if [ ! -f $BUTTERFLY_BUILD_ROOT/packetgraph/tests/tests ]; then
    echo "Please run script from the build directory"
    exit 1
fi

# Test Butterfly root
if [ ! -d $BUTTERFLY_ROOT/packetgraph ]; then
    echo "Please set butterfly's source root as parameter"
    exit 1
fi

GRAPH=$BUTTERFLY_BUILD_ROOT/packetgraph/tests/tests

if [ ! -f $GRAPH ]; then
	echo "$GRAPH does not exists"
	exit 1
fi

# Run the packetgraph tests in quick mode
$GRAPH -c1 -n1 --socket-mem 64 --vdev=eth_pcap0,rx_pcap=$TEST_DIR/enter.pcap,tx_pcap=$TEST_DIR/out.pcap --vdev=eth_ring0 -- -quick-test || { echo 'failed' ; exit 1; }

