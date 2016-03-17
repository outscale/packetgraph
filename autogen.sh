#!/bin/sh

libtoolize -c --force --install && aclocal && automake -c --add-missing && autoconf
