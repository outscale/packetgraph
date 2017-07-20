#!/bin/bash

if ! [ -z $1 ]; then
    cd $1;
fi
libtoolize -c --force --install && aclocal && automake -c --add-missing && autoconf

if ! ./dpdk_symbols.sh ; then
    rm ./configure
    exit 1;
fi

if ! [ -z $1 ]; then
    cd -;
fi
