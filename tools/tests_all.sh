#!/bin/bash

# Copyright 2015 Outscale SAS
#
# This file is part of Packetgraph.
#
# Packetgraph is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as published
# by the Free Software Foundation.
#
# Packetgraph is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.

usage="tests_all.sh BUILD_ROOT PACKETGRAPH_ROOT"

# Test number of arguments
if [ "$#" -ne 2 ]; then
	echo echo $usage
	exit 1
fi

BUILD_ROOT=$(readlink -m $1)
PACKETGRAPH_ROOT=$(readlink -m $2)

for n in build-core build-diode build-print build-firewall build-hub build-nic build-switch build-vhost build-vtep build-antispoof; do
    d=$(echo $n | sed -e 's/build-//')
    cd $BUILD_ROOT/$n/
    tput setaf 2
    echo [ ------ test $n ------ ]
    tput setaf 7
    make tests
    if [ $? != 0 ]; then
	tput setaf 1
	cowsay -h &> /dev/null
	HAS_COWSAY=$?
	ponysay test &> /dev/null
	HAS_PONY=$?
	if [ $HAS_PONY == 0 ]; then
	    ponysay -f derpysad "test $d fail"
	elif [ $HAS_COWSAY != 127 ]; then
	    echo test $d fail ! | cowsay -d
	else
	    echo X ------ test $d fail ------ X
	fi
	tput setaf 7
	exit 1
    fi
done
