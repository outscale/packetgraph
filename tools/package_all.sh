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

set -e

usage="package_all.sh PACKETGRAPH_ROOT"

# Test number of arguments
if [ "$#" -ne 1 ]; then
	echo echo $usage
	exit 1
fi

PACKETGRAPH_ROOT=$(readlink -m $1)
p=$(pwd)

# Test RTE_SDK
if [ -z $RTE_SDK ]; then
	echo "please set RTE_SDK environment variable"
	exit 1
fi

BRICKS="core diode print firewall hub nic switch vhost vtep"

export PG_CORE="$p/core"
export PG_HUB="$p/hub"
for n in $BRICKS; do
	build=$p/$n
	mkdir $build | true
	cd $build
	cmake ${PACKETGRAPH_ROOT}/$n
	make
	${PACKETGRAPH_ROOT}/tools/package.sh ${PACKETGRAPH_ROOT}/$n $build $n
	mv $build/*.rpm $p/
done

for n in $BRICKS; do
	rm -rf $p/$n
done
