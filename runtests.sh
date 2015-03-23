#!/bin/sh

# Launch the tests suites of all the subprojects

GRAPH=packetgraph/tests/tests

if [ ! -f $GRAPH ]; then
	echo "$GRAPH does not exists"
	exit 1
fi

# run the packetgraph tests in quick mode
$GRAPH -c1 -n1 --socket-mem 64 -- -quick-test || { echo 'failed' ; exit 1; }

echo "All test succeded"

exit 0

