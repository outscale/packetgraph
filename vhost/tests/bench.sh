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

usage="usage: bench.sh VHOST_DIR_PATH BUILD_DIR_PATH"

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

# Test that qemu is installed
if [ "$(whereis qemu-system-x86_64 | wc -w)" == "1" ]; then
	echo "need qemu-system-x86_64 to be installed";
	echo $usage
	exit 1
fi

# Check that huge pages mount exists
if [ ! -d /mnt/huge ]; then
	echo "Is your /mnt/huge is mounted with hugepages ?"
	exit 1
fi

# Check that buildroot is built
if [ ! -f $BUILD_ROOT/buildroot/BUILDROOT_OK ]; then
	$VHOST_ROOT/tests/build_buildroot.sh $VHOST_ROOT $BUILD_ROOT
fi

# Clean old sockets
sudo rm /tmp/qemu-vhost-* | true

# Check hugepages
NB_HUGE=`grep HugePages_Free /proc/meminfo | awk -F':' '{ print $2 }'`

echo -----------
echo nb free huge page $NB_HUGE
echo -----------

if [  $NB_HUGE -lt 50 ]; then
   tput setaf 1
   echo not enouth free hugepage, should have more than 50
   tput setaf 7
   exit 1
fi

echo $BUILD_ROOT/
file $BUILD_ROOT/packetgraph-vhost-bench
# Launch test
sudo $BUILD_ROOT/packetgraph-vhost-bench -c1 -n1 --socket-mem 64 -- -bzimage $BUILD_ROOT/buildroot/output/images/bzImage -cpio $BUILD_ROOT/buildroot/output/images/rootfs.cpio -hugepages /mnt/huge

