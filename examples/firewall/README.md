# Firewall example

This example shows an examples of using the firewall brick between two physical
nics using packetgraph and DPDK:

```
[nic]---[firewall]---[nic]
```

# Build

Just add `--with-examples` to your ./configure options and run make.
You can run example with a simple script:
```
./examples/firewall/run.sh
```

# Configure your NICs

## Using DPDK compatible NICs

If you are using DPDK nics, you have to bind them to a DPDK compatible driver
so they are not managed by the kernel anymore.

To show the current status of your NICs, we will use a script in DPDK project:
```
$ ./dpdk/tools/dpdk_nic_bind.py --status

Network devices using DPDK-compatible driver
============================================
<none>

Network devices using kernel driver
===================================
0000:02:00.0 '82599ES 10-Gigabit SFI/SFP+ Network Connection' if=enp2s0f0 drv=ixgbe unused=
0000:02:00.1 '82599ES 10-Gigabit SFI/SFP+ Network Connection' if=enp2s0f1 drv=ixgbe unused=
0000:03:00.0 '82574L Gigabit Network Connection' if=enp3s0 drv=e1000e unused= *Active*
0000:04:00.0 '82574L Gigabit Network Connection' if=enp4s0 drv=e1000e unused= *Active*

Other network devices
=====================
<none>
```

Here, we have:
- 2x 1 Gigabit cards
- 1x 10 Gigabit card (DPDK compatible) with two ports

To bind them, we will have to ensure we have loaded vfio-pci kernel module:
```
$ modprobe vfio-pci
```

Then, we will bind our two ports to DPDK using vfio driver:

```
$ ./dpdk/tools/dpdk_nic_bind.py --bind=vfio-pci enp2s0f0
$ ./dpdk/tools/dpdk_nic_bind.py --bind=vfio-pci enp2s0f1
$ ./dpdk/tools/dpdk_nic_bind.py --status

Network devices using DPDK-compatible driver
============================================
0000:02:00.0 '82599ES 10-Gigabit SFI/SFP+ Network Connection' drv=vfio-pci unused=
0000:02:00.1 '82599ES 10-Gigabit SFI/SFP+ Network Connection' drv=vfio-pci unused=

Network devices using kernel driver
===================================
0000:03:00.0 '82574L Gigabit Network Connection' if=enp3s0 drv=e1000e unused=vfio-pci *Active*
0000:04:00.0 '82574L Gigabit Network Connection' if=enp4s0 drv=e1000e unused=vfio-pci *Active*

Other network devices
=====================
<none>
```

Our two ports are now not managed anymore by the kernel are now ready to be used
in our firewall example.

## Using non-DPDK NICs

We can also use NICs which are not optimized to be used with DPDK.
This will just requiere to pass some additional DPDK arguments to our example:
"--vdev=eth_pcap0,iface=eth0 --vdev=eth_pcap1,iface=eth1".

# Run example

You can just run the firewall like:
```
$ ./examples/firewall/run.sh
```

The program will use the first two DPDK ports available.

You can also configure the cores where the firewall should run. To do this,
you should isolate a CPU from your linux using the kernel parameter
[isolcpu](http://www.linuxtopia.org/online_books/linux_kernel/kernel_configuration/re46.html).
Once you have your core isolated, you can run the firewall using ```taskset```
command.

For example, you can set isolcpu=0 and run the firewall on the first core:
```
$ taskset -c 0 ./examples/firewall/run.sh
```

# Test firewall

As you may saw in example sources, the firewall is configured to accept ICMP and
TCP packets in port range 50-60.
You can connect two other hosts on each physical nics and try to make some
traffic tests using iperf for example.
Note that firewall only manage IP protocols and is not meant to filter other
ethernet level protocols such as ARP.

