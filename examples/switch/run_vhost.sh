#!/bin/bash
IMG_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-260417.qcow
IMG_MD5=7bd77480631145b679bc8fac3583b163
KEY_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-260417.rsa
KEY_MD5=eb3d700f2ee166e0dbe00f4e0aa2cef9

declare -A qemu_pids

function ssh_run {
    id=$1
    cmd=("${@:2}")
    # shellcheck disable=SC2029
    ssh -q -p 500"$id" -l root -i vm.rsa -oStrictHostKeyChecking=no 127.0.0.1 "${cmd[@]}"
}

function ssh_run_timeout {
    id=$1
    timeout=$2
    cmd=("${@:3}")
    # shellcheck disable=SC2029
    ssh -q -p 500"$id" -l root -i vm.rsa -oStrictHostKeyChecking=no -oConnectTimeout="$timeout" 127.0.0.1 "${cmd[@]}"
}

function ssh_bash {
    id=$1
    ssh -p 500"$id" -l root -i vma.rsa -oStrictHostKeyChecking=no 127.0.0.1
}

function ssh_ping {
    id1=$1
    id2=$2
    ssh_run "$id1" ping 42.0.0."$id2" -c 1 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "ping $id1 --> $id2 FAIL"
        exit 1
    else
        echo "ping $id1 --> $id2 OK"
    fi
}

function ssh_no_ping {
    id1=$1
    id2=$2
    ssh_run "$id1" ping 42.0.0."$id2" -c 1 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "no ping $id1 --> $id2 OK"
    else
        echo "no ping $id1 --> $id2 FAIL"
        exit 1
    fi
}

function qemu_start {
    id=$1
    echo "starting VM $id"
    port=$((id - 1))
    SOCKET_PATH=/tmp/qemu-vhost$port
    IMG_PATH=vm.qcow
    MAC=52:54:00:12:34:0$id
    sudo qemu-system-x86_64 -redir tcp:500"${id}"::22 -netdev user,id=network0 -device e1000,netdev=network0 -m 256M -enable-kvm -chardev socket,id=char0,path="$SOCKET_PATH" -netdev type=vhost-user,id=mynet1,chardev=char0,vhostforce -device virtio-net-pci,mac="$MAC",netdev=mynet1 -object memory-backend-file,id=mem,size=256M,mem-path=/mnt/huge,share=on -numa node,memdev=mem -mem-prealloc -drive file="$IMG_PATH" -snapshot -nographic &
    pid=$!
    kill -s 0 $pid &> /dev/null
    if [ $? -ne 0 ]; then
        echo "failed to start qemu"
        exit 1
    fi

    qemu_pids["$id"]=$pid

    # Wait for ssh to be ready
    while ! ssh_run_timeout "$id" 60 true ; do
        sleep 1
    done

    # Configure IP on vhost interface
    ssh_run "$id" ip link set ens4 up
    ssh_run "$id" ip addr add 42.0.0."$id"/16 dev ens4

    echo "You can now connect on VM $id with:"
    echo "$ ssh root@127.0.0.1 -p 500$id -i vm.rsa"
}

function qemu_stop {
    id=$1
    echo "stopping VM $id"
    sudo kill -9 "$(ps --ppid ${qemu_pids[$id]} -o pid=)"
}

function download {
    url=$1
    md5=$2
    path=$3

    if [ ! -f "$path" ]; then
        echo "$path, let's download it..."
        wget "$url" -O "$path"
    fi

    if [ ! "$(md5sum "$path" | cut -f 1 -d ' ')" == "$md5" ]; then
        echo "Bad md5 for $path, let's download it ..."
        wget "$url" -O "$path"
        if [ ! "$(md5sum "$path" | cut -f 1 -d ' ')" == "$md5" ]; then
            echo "Still bad md5 for $path ... abort."
            exit 1
        fi
    fi
}

function check_bin {
    run=("${@:1}")
    "${run[@]}" &> /dev/null
    if [ ! "$?" == "0" ]; then
        echo "cannot execute " "${run[@]}" ": not found"
        exit 1
    fi
}

if ! test -f ./example-switch ; then
    echo "example-switch not found, do you run this script in you build directory ?"
    exit 1
fi

download $IMG_URL $IMG_MD5 vm.qcow
download $KEY_URL $KEY_MD5 vm.rsa
chmod og-r vm.rsa

# Check some binaries
check_bin sudo -h
check_bin ssh -V
check_bin sudo qemu-system-x86_64 -h
check_bin sudo socat -h

sudo ./example-switch -c1 -n1 --socket-mem 64 --no-shconf -- --vhost 2 "$@" &
example_pid=$!
sleep 10
qemu_start 1
qemu_start 2
ssh_ping 1 2
ssh_ping 2 1
sleep 600
qemu_stop 1
qemu_stop 2
kill -9 $example_pid
