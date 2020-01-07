#!/bin/bash

cd $TRAVIS_BUILD_DIR
git config user.name "$GH_USER_NAME"
git config user.email "$GH_USER_MAIL"
export LD="llvm-link"
./configure -p --with-benchmarks --with-examples

make style

set -x
set -e

if [ "$(whoami)" == "travis" ]; then
    if [ ! -e "/usr/bin/doxygen" ]; then
	sudo apt install -y doxygen
    fi
    make doc
fi
