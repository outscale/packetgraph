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


# Check that huge pages mount exists
if [ ! -d /mnt/huge ]; then
	echo "Is your /mnt/huge is mounted with hugepages ?"
	exit 1
fi

# Clean old sockets
sudo rm /tmp/qemu-vhost-* | true

# Check hugepages
NB_HUGE=$(grep HugePages_Free /proc/meminfo | awk -F':' '{ print $2 }')

echo -----------
echo nb free huge page "$NB_HUGE"
echo -----------

if [  "$NB_HUGE" -lt 1 ]; then
   tput setaf 1
   echo not enouth free hugepage, should have more than 1
   tput setaf 7
   exit 1
fi

# Launch test
#gdb --args tests-srv -c1 -n1 --socket-mem 64 --no-shconf -- -hugepages /mnt/huge


sudo ./tests-srv -c1 -n1 --no-shconf --no-huge -- -hugepages /mnt/huge




