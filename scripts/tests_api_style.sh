#!/bin/sh

# Butterfly root
BUTTERFLY_ROOT=$1
BUTTERFLY_BUILD_ROOT=.

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

sources="$BUTTERFLY_ROOT/api/client/client.cc \
$BUTTERFLY_ROOT/api/server/app.cc \
$BUTTERFLY_ROOT/api/server/app.h \
$BUTTERFLY_ROOT/api/server/server.cc \
$BUTTERFLY_ROOT/api/server/server.h \
$BUTTERFLY_ROOT/api/server/model.cc \
$BUTTERFLY_ROOT/api/server/model.h"


$BUTTERFLY_ROOT/scripts/cpplint.py --filter=-build/c++11 --root=$BUTTERFLY_ROOT $sources
if [ $? != 0 ]; then
    echo "${RED}API style test failed${NORMAL}"
    exit 1
fi

cppcheck --check-config --error-exitcode=1 --enable=all -I $BUTTERFLY_ROOT $sources
if [ $? != 0 ]; then
    echo "${RED}API style test failed${NORMAL}"
    exit 1
fi

exit 0
