set -x
sudo qemu-system-x86_64 vm.qcow -display none \
-netdev user,id=network0,hostfwd=tcp::500${id}-:22 -device e1000,netdev=network0 \
 -name vm2 -enable-kvm -cpu host -smp 2 -m 128 \
-object memory-backend-file,id=mem,size=128M,mem-path=/mnt/huge,share=on \
-numa node,memdev=mem -mem-prealloc  \
-chardev socket,id=char0,path=/tmp/qemu-vhost-X,server \
-netdev type=vhost-user,id=netdev0,chardev=char0,vhostforce \
-device virtio-net-pci,netdev=netdev0,mac=52:54:00:00:00:01,mrg_rxbuf=on,rx_queue_size=1024,tx_queue_size=1024 \

#-monitor unix:/tmp/vm2_monitor.sock,server,nowait -net nic,vlan=2,macaddr=00:00:00:08:e8:aa,addr=1f \
#-chardev socket,id=char0,path=./vhost-net,server \
#-netdev type=vhost-user,id=netdev0,chardev=char0,vhostforce \
#-device virtio-net-pci,netdev=netdev0,mac=52:54:00:00:00:01,mrg_rxbuf=on,rx_queue_size=1024,tx_queue_size=1024 \

#-object memory-backend-file,id=mem,size=4096M,mem-path=/mnt/huge,share=on \
#-chardev socket,path=/tmp/qemu-vhost-X,server,nowait,id=vm2_qga0 -device virtio-serial \
#-chardev socket,id=char0,path=./vhost-net,server \
#-netdev type=vhost-user,id=netdev0,chardev=char0,vhostforce \
#-device virtio-net-pci,netdev=netdev0,mac=52:54:00:00:00:01,mrg_rxbuf=on,rx_queue_size=1024,tx_queue_size=1024 \


# -device virtserialport,chardev=vm2_qga0,name=org.qemu.guest_agent.2 -daemonize \
# -monitor unix:/tmp/vm2_monitor.sock,server,nowait -net nic,vlan=2,macaddr=00:00:00:08:e8:aa,addr=1f \

#qemu-system-x86_64 -serial stdio -name vm2 -enable-kvm -cpu host -smp 2 -m 4096 \

#-chardev socket,path=/tmp/qemu-vhost-X,server,nowait,id=vm2_qga0 -device virtio-serial \
#-device virtserialport,chardev=vm2_qga0,name=org.qemu.guest_agent.2 \

