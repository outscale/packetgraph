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

# Get VM image & key
IMG_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-100816.qcow
IMG_MD5=1ca000ddbc5ac271c77d1875fab71083
KEY_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-100816.rsa
KEY_MD5=eb3d700f2ee166e0dbe00f4e0aa2cef9
download $IMG_URL $IMG_MD5 vm.qcow
download $KEY_URL $KEY_MD5 vm.rsa
chmod og-r vm.rsa

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

# Launch test
sudo ./tests-vhost -c1 -n1 --socket-mem 64 -- -vm vm.qcow -vm-key vm.rsa -hugepages /mnt/huge

