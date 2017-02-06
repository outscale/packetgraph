#!/bin/bash
set -e
if ! [ -z $1 ]; then
    cd $1;
fi
libtoolize -c --force --install && aclocal && automake -c --add-missing && autoconf && ./dpdk_symbols.sh
if ! [ -z $1 ]; then
    cd -;
fi
