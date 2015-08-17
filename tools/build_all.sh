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

usage="build_all.sh PACKETGRAPH_ROOT"

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

for n in build-core build-diode build-print build-firewall build-hub build-nic build-switch build-vhost build-vtep build-antispoof; do
	build=$p/$n
	mkdir $build | true
	cd $build
	d=$(echo $n | sed -e 's/build-//')
	export PG_CORE="$p/build-core"
	export PG_HUB="$p/build-hub"
	cmake ${PACKETGRAPH_ROOT}/$d
	make
done
