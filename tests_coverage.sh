#!/bin/bash

# Packetgraph sources directory
PACKETGRAPH_ROOT=$(cd "$(dirname $0)" && pwd) 
PACKETGRAPH_SRC_ROOT=$PACKETGRAPH_ROOT/src/


# Test coverage data files
ret=$(find $PACKETGRAPH_SRC_ROOT -name '*.gcda' | wc -l)
if [ $ret -eq 0 ]; then
    echo " Coverage files (*.gcda) not found in $PACKETGRAPH_SRC_ROOT"
    echo " Make sure you did that in your packetgraph root directory:"
    echo " - ./configure  -c or --with-coverage"
    echo " - make"
    echo " run some tests before running coverage test."
    exit 1
fi

rm -fr $PACKETGRAPH_ROOT/packetgraph_coverage

function clean_coverage_data {
    find $PACKETGRAPH_SRC_ROOT/ -name '*.gc*' -exec rm -fr {} \; || exit 1
    find $PACKETGRAPH_ROOT/tests/* -name '*.gc*' -exec rm -fr {} \; || exit 1
    rm packetgraph.info
}

lcov -h >/dev/null || (echo "ERROR: lcov command not found" ; exit 1)
genhtml -h >/dev/null || (echo "ERROR: genhtml command not found" ; exit 1)

lcov -c -d $PACKETGRAPH_SRC_ROOT -o packetgraph.info
genhtml $PACKETGRAPH_ROOT/packetgraph.info -o packetgraph_coverage > /dev/null || exit 1
gcovr -r 'src/' --xml-pretty -o coverage.xml > /dev/null || exit 1

if [ "$(whoami)" != "jenkins" ]; then
    clean_coverage_data
fi
echo COVERAGE TEST OK 
exit 0
