#!/bin/bash

cd $RTE_SDK/

sudo docker run  --privileged -v  /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /mnt/huge:/mnt/huge -v /sys/devices/system/node:/sys/devices/system/node   -v /tmp:/tmp -v /dev/hugepages:/dev/hugepages dpdk-app-testpmd vhost-srv.sh #example-dperf  -m 500 -n 4  --no-pci     --vdev=virtio_user0,path=/var/run/usvhost --file-prefix=container --no-pci -l 3    -- --sip 42.0.0.1 --dip 42.0.0.2 --smac 52:54:00:12:34:01 --dmac 52:54:00:12:34:02 --server --time 20
