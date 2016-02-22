#!/bin/bash

SWITCH_SRC_ROOT=$1
SWITCH_BUILD_ROOT=$2

source $SWITCH_SRC_ROOT/functions.sh

network_connect 0 1
server_start 0
sleep 4
qemu_start 1
qemu_start 2
ssh_ping 1 2
ssh_ping 2 1
sleep 1000000
qemu_stop 1
qemu_stop 2
server_stop 0
network_disconnect 0 1
return_result

