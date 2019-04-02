#!/bin/bash

cd $TRAVIS_BUILD_DIR
git config user.name "$(GH_USER_NAME)"
git config user.email "$(USER_MAIL)"
export LD="llvm-link"
./configure -p --with-benchmarks --with-examples

make style

set -x
set -e

if [ "$(whoami)" == "travis" ]; then
    if [ ! -e "/usr/bin/doxygen" ]; then
	sudo curl https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/doxygen.sh -o /usr/bin/doxygen
	sudo chmod +x /usr/bin/doxygen
    fi
    make doc
fi
