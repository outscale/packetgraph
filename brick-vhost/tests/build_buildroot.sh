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

# Butterfly root
VHOST_ROOT=$1
BUILD_ROOT=$2

usage="usage: build_buildroot.sh VHOST_DIR_PATH BUILD_DIR_PATH"

# Test vhost root
if [ ! -f $VHOST_ROOT/src/vhost.c ]; then
	echo $usage
	exit 1
fi

# Test build root
if [ ! -d $BUILD_ROOT/CMakeFiles ]; then
	echo $usage
	exit 1
fi


# Test if buildroot has been cloned
if [ ! -d $BUILD_ROOT/buildroot ]; then
	cd $BUILD_ROOT
	git clone git://git.buildroot.net/buildroot
	cd buildroot
fi

# Patch buildroot for config & build
cd $BUILD_ROOT/buildroot
patch -t -N -p1 < $VHOST_ROOT/tests/0001-config-custom-buildroot-config-for-testing.patch || true
chmod +x overlay/sbin/init
yes "" | make oldconfig
make
touch $BUILD_ROOT/buildroot/BUILDROOT_OK
