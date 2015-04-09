#!/bin/sh

# Butterfly root
BUTTERFLY_ROOT=$1
BUTTERFLY_BUILD_ROOT=.

# Test Butterfly build root
if [ ! -f $BUTTERFLY_BUILD_ROOT/api/client/butterfly-client ]; then
    echo "Please run script from the build directory"
    exit 1
fi

# Test Butterfly root
if [ ! -d $BUTTERFLY_ROOT/packetgraph ]; then
    echo "Please set butterfly's source root as parameter"
    exit 1
fi

run=$BUTTERFLY_ROOT/api/tests/run_scenario.sh
client=$BUTTERFLY_BUILD_ROOT/api/client/butterfly-client
server=$BUTTERFLY_BUILD_ROOT/api/server/butterfly-server
err=false
for request in $BUTTERFLY_ROOT/api/tests/*/*_in; do
    expected=$(echo $request | sed -e 's/_in$/_out/')
    $run $client $server $request $expected
    if [ $? -ne 0 ]; then
        echo "$(basename $request) scenario failed"
        err=true
    else
        echo "$(basename $request) scenario OK"
    fi
done

if [ $err = true ]; then
    exit 1
fi
exit 0
