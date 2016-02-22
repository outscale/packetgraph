IMG_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-051115.qcow
IMG_MD5=4b7b1a71ac47eb73d85cdbdc85533b07
KEY_URL=https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/arch-051115.rsa
KEY_MD5=eb3d700f2ee166e0dbe00f4e0aa2cef9

function usage {
    echo "Usage: test.sh SWITCH_SRC_ROOT SWITCH_BUILD_ROOT" 1>&2
}

declare -A qemu_pids
declare -A server_pids
declare -A socat_pids

RETURN_CODE=0

function return_result {
    exit $RETURN_CODE
}

function ssh_run {
    id=$1
    cmd="${@:2}"
    key=$SWITCH_BUILD_ROOT/vm.rsa
    ssh -q -p 500$id -l root -i $key -oStrictHostKeyChecking=no 127.0.0.1 $cmd 
}

function ssh_run_timeout {
    id=$1
    timeout=$2
    cmd="${@:3}"
    key=$SWITCH_BUILD_ROOT/vm.rsa
    ssh -q -p 500$id -l root -i $key -oStrictHostKeyChecking=no -oConnectTimeout=$timeout 127.0.0.1 $cmd 
}

function ssh_bash {
    id=$1
    cmd="${@:2}"
    key=$SWITCH_BUILD_ROOT/vm.rsa
    ssh -p 500$id -l root -i $key -oStrictHostKeyChecking=no 127.0.0.1
}

function ssh_ping {
    id1=$1
    id2=$2
    ssh_run $id1 ping 42.0.0.$id2 -c 1 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "ping $id1 --> $id2 FAIL"
        RETURN_CODE=1
    else
        echo "ping $id1 --> $id2 OK"
    fi
}

function ssh_no_ping {
    id1=$1
    id2=$2
    ssh_run $id1 ping 42.0.0.$id2 -c 1 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "no ping $id1 --> $id2 OK"
    else
        echo "no ping $id1 --> $id2 FAIL"
        RETURN_CODE=1
    fi
}

function qemu_start {
    id=$1
    echo "starting VM $id"
    SOCKET_PATH=/tmp/qemu-vhost-$id
    IMG_PATH=$SWITCH_BUILD_ROOT/vm.qcow
    MAC=52:54:00:12:34:0$id

    CMD="sudo qemu-system-x86_64 -redir tcp:500${id}::22 -netdev user,id=network0 -device e1000,netdev=network0 -m 256M -enable-kvm -chardev socket,id=char0,path=$SOCKET_PATH -netdev type=vhost-user,id=mynet1,chardev=char0,vhostforce -device virtio-net-pci,mac=$MAC,netdev=mynet1 -object memory-backend-file,id=mem,size=256M,mem-path=/mnt/huge,share=on -numa node,memdev=mem -mem-prealloc -drive file=$IMG_PATH -snapshot -nographic"
    exec $CMD  &
    pid=$!
    kill -s 0 $pid &> /dev/null
    if [ $? -ne 0 ]; then
        echo "failed to start qemu"
        exit 1
    fi

    qemu_pids["$id"]=$pid

    # Wait for ssh to be ready
    while ! ssh_run_timeout $id 60 true ; do
        sleep 1
    done

    # Configure IP on vhost interface
    ssh_run $id ip link set ens4 up
    ssh_run $id ip addr add 42.0.0.$id/16 dev ens4
}

function qemu_stop {
    id=$1
    echo "stopping VM $id"
    sudo kill -9 $(ps --ppid ${qemu_pids[$id]} -o pid=)
}

function server_start {
    id=$1
    echo "starting butterfly $id"

    # Remove lock file to run other butterfly on the same host
    # Don't do this at home
    sudo rm -rf /var/run/.rte_config &> /dev/null

    chmod a+x $SWITCH_BUILD_ROOT/switch
    CMD="sudo valgrind --tool=callgrind $SWITCH_BUILD_ROOT/switch -cf -n1 --socket-mem 64 -- -vhost 4"
    exec $CMD &
    pid=$!
    sudo kill -s 0 $pid
    if [ $? -ne 0 ]; then
        echo "failed to start butterfly"
        exit 1
    fi

    server_pids["$id"]=$pid
}

function server_stop {
    id=$1
    echo "stopping butterfly $id"
    sudo kill -9 $(ps --ppid ${server_pids[$id]} -o pid=)
}

function network_connect {
    id1=$1
    id2=$2
    echo "network connect but$id1 <--> but$id2"
    sudo socat TUN:192.168.42.$id1/24,tun-type=tap,iff-running,iff-up,iff-promisc,tun-name=but$id1 TUN:192.168.42.$id2/24,tun-type=tap,iff-running,iff-promisc,iff-up,tun-name=but$id2 &
    pid=$!
    sleep 1
    sudo kill -s 0 $pid
    if [ $? -ne 0 ]; then
        echo "failed connect but$id1 and but$id2"
        exit 1
    fi
    socat_pids["$id1$id2"]=$pid
}

function network_disconnect {
    id1=$1
    id2=$2
    echo "network disconnect but$id1 <--> but$id2"
    sudo kill -9 $(ps --ppid ${socat_pids[$id1$id2]} -o pid=)
}

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

function check_bin {
    run=${@:1}
    $run &> /dev/null
    if [ ! "$?" == "0" ]; then
        echo "cannot execute $run: not found"
        exit 1
    fi
}

if [ ! -f $SWITCH_BUILD_ROOT/switch ]; then
    echo "Switch's build root not found (is switch built ?)"
    usage
    exit 1
fi

download $IMG_URL $IMG_MD5 $SWITCH_BUILD_ROOT/vm.qcow
download $KEY_URL $KEY_MD5 $SWITCH_BUILD_ROOT/vm.rsa
chmod og-r $SWITCH_BUILD_ROOT/vm.rsa

# Check some binaries
check_bin sudo -h
check_bin ssh -V
check_bin sudo qemu-system-x86_64 -h
check_bin sudo socat -h

# run sudo one time
sudo echo "ready to roll !"

