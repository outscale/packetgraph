#!/bin/sh
if ! [ -z $1 ]; then
    cd $1;
fi
libtoolize -c --force --install && aclocal && automake -c --add-missing && autoconf
if ! [ -z $1 ]; then
    cd -;
fi
