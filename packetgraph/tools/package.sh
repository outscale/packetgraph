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

usage="package.sh BRICK_ROOT BRICK_BUILD_ROOT BRICK_NAME"

# Test number of arguments
if [ "$#" -ne 3 ]; then
	echo echo $usage
	exit 1
fi

BRICK_ROOT=$(readlink -m $1)
BRICK_BUILD_ROOT=$(readlink -m $2)
BRICK_NAME=$3

# Test RTE_SDK
if [ -z $RTE_SDK ]; then
	echo "please set RTE_SDK environment variable"
	exit 1
fi

# Test brick root
if [ ! -d $BRICK_ROOT/src ]; then
	echo $usage
	exit 1
fi

# Test build root
if [ ! -d $BRICK_BUILD_ROOT/CMakeFiles ]; then
	echo $usage
	exit 1
fi

make -C $BRICK_BUILD_ROOT package &> /dev/null

# .spec files are for the moment generated with conflicting %dir entries
# Futur versions of cmake removes this conflict.

# Manually remove %dir entries
cat ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM/SPECS/packetgraph-${BRICK_NAME}.spec | grep -v "^%dir" > /tmp/build-${BRICK_NAME}
mv /tmp/build-${BRICK_NAME} ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM/SPECS/packetgraph-${BRICK_NAME}.spec

# Manually rebuild package
/usr/bin/rpmbuild -bb --define "_topdir ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM" --buildroot ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM/packetgraph-${BRICK_NAME}-0.0.1-1.x86_64 ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM/SPECS/packetgraph-${BRICK_NAME}.spec &> /dev/null

# Move package
mv ${BRICK_BUILD_ROOT}/_CPack_Packages/Linux/RPM/packetgraph-${BRICK_NAME}-0.0.1-1.x86_64.rpm ${BRICK_BUILD_ROOT}/
