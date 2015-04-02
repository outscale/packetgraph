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

tmp=$BUTTERFLY_BUILD_ROOT/.tmp_tests_packetgraph_style
echo "0" > ${tmp}_ret
find $BUTTERFLY_ROOT/packetgraph -name "*.[ch]" | while read f; do
    echo "checking $f..."
    $BUTTERFLY_ROOT/scripts/checkpatch.pl -q -f $f 2> /dev/null > $tmp
    if [ ! -z "$(cat $tmp)" ]; then
        cat $tmp
        echo "1" > ${tmp}_ret
    fi
done
ret=$(cat ${tmp}_ret)
rm $tmp ${tmp}_ret
exit $ret
