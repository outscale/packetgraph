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

c_filelist=$(find $BUTTERFLY_ROOT/packetgraph -type f -name "*.c" -printf %p\ )
h_filelist=$(find $BUTTERFLY_ROOT/packetgraph -type f -name "*.h" -printf %p\ )
$BUTTERFLY_ROOT/scripts/checkpatch.pl -q -f $c_filelist $h_filelist
if [ $? != 0 ]; then
    echo "checkpatch tests failed"
    exit 1
fi

make lizard-run
if [ $? != 0 ]; then
    echo "lizard tests failed"
    exit 1
fi

cppcheck > /dev/null
if [ $? != 0 ]; then
    echo "cppcheck is not installed, some tests will be skipped"
else
    cppcheck -q  -f -I $BUTTERFLY_ROOT/packetgraph/include --error-exitcode=43 --enable=style --enable=performance --enable=portability --enable=information --enable=missingInclude --enable=warning $c_filelist
    if [ $? != 0 ]; then
	echo "cppcheck tests failed"
	exit 1
    fi
fi

rats > /dev/null
if [ $? != 0 ]; then
    echo "rats is not installed, some tests will be skipped"
else
    make rats
    if [ $? != 0 ]; then
	echo "rats tests failed"
	exit 1
    fi
fi

flawfinder > /dev/null
if [ $? != 0 ]; then
    echo "flawfinder is not installed, some tests will be skipped"
else
    make flawfinder
    if [ $? != 0 ]; then
	echo "flawfinder tests failed"
	exit 1
    fi
fi

exit 0
