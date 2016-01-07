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
VHOST_BUILD_ROOT=$2
BUILD_ROOT=$3

usage="usage: build_buildroot.sh VHOST_DIR_PATH VHOST_BUILD_DIR_PATH BUILD_ROOT"

function download {
    url=$1
    md5=$2
    path=$3

    if [ ! -f $path ]; then
        echo "$path, let's download it..."
        wget $url -O $path
    fi

    if [ ! "$(md5sum $path | cut -f 1 -d ' ')" == "$md5" ]; then
        echo "Bad md5 for $path, let's download it ..."
        wget $url -O $path
        if [ ! "$(md5sum $path | cut -f 1 -d ' ')" == "$md5" ]; then
            echo "Still bad md5 for $path ... abort."
            exit 1
        fi
    fi
}

bin="packetgraph-integration-tests"

# Test vhost root
if [ ! -f $VHOST_ROOT/src/vhost.c ]; then
	echo can not found $VHOST_ROOT/src/vhost.c
	echo $usage
	exit 1
fi

# Test vhost build root
if [ ! -d $VHOST_BUILD_ROOT/CMakeFiles ]; then
	echo can not found $VHOST_BUILD_ROOT/CMakeFiles
	echo $usage
	exit 1
fi

# Test build root
if [ ! -f $BUILD_ROOT/$bin ]; then
	echo can not found $BUILD_ROOT/$bin
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
if [ ! -f $VHOST_BUILD_ROOT/buildroot/BUILDROOT_OK ]; then
	$VHOST_ROOT/tests/build_buildroot.sh $VHOST_ROOT $VHOST_BUILD_ROOT
fi

# Get VM image & key
IMG_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-051115.qcow
IMG_MD5=4b7b1a71ac47eb73d85cdbdc85533b07
KEY_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-051115.rsa
KEY_MD5=eb3d700f2ee166e0dbe00f4e0aa2cef9
download $IMG_URL $IMG_MD5 $BUILD_ROOT/vm.qcow
download $KEY_URL $KEY_MD5 $BUILD_ROOT/vm.rsa
chmod og-r $BUILD_ROOT/vm.rsa

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

echo $VHOST_BUILD_ROOT/
file $VHOST_BUILD_ROOT/packetgraph-vhost-tests
# Launch test
sudo $BUILD_ROOT/$bin -c1 -n1 --socket-mem 64 -- -vm $BUILD_ROOT/vm.qcow -vm-key $BUILD_ROOT/vm.rsa -hugepages /mnt/huge
