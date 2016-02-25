#!/bin/sh
sudo yum install -y cmake git gcc gcc-c++ glib2-devel libpcap-devel rpm-build libtool kernel-devel libpfm-devel zlib-devel openssl-devel flex byacc

git clone git://dpdk.org/dpdk
cd dpdk
git checkout -b v2.2.0 v2.2.0
echo "export RTE_SDK=$HOME/dpdk" >> ~/.bashrc
source ~/.bashrc
make config T=x86_64-native-linuxapp-gcc
make -j $(nproc) CONFIG_RTE_LIBRTE_VHOST=y CONFIG_RTE_LIBRTE_PMD_PCAP=y CONFIG_RTE_EAL_IGB_UIO=n CONFIG_RTE_LIBRTE_KNI=n CONFIG_RTE_KNI_KMOD=n EXTRA_CFLAGS='-g'
cd ~

wget http://noxt.eu/~rmind/libprop-0.6.5-1.el7.centos.x86_64.rpm -O libprop.rpm
sudo rpm -i libprop.rpm
rm libprop.rpm

#git clone https://github.com/rmind/libcdb
#cd libcdb
#make rpm
#rpm -ihv RPMS/x86_64/libcdb-*
#cd ~
wget https://osu.eu-west-2.outscale.com/jerome.jutteau/89esf1wd698h6sd64n4qa6k6m45/libcdb-1.0-1.el7.centos.x86_64.rpm -O libcdb.rpm
sudo rpm -i libcdb.rpm
rm libcdb.rpm

git clone https://github.com/rmind/libqsbr
cd libqsbr/pkg
make
sudo rpm -i RPMS/x86_64/libqsbr-*.rpm
cd ~

wget https://osu.eu-west-2.outscale.com/jerome.jutteau/89esf1wd698h6sd64n4qa6k6m45/libbpfjit-0.1-1.x86_64.rpm -O libbpfjit.rpm
sudo rpm -i libbpfjit.rpm
rm libbpfjit.rpm

wget https://osu.eu-west-2.outscale.com/jerome.jutteau/89esf1wd698h6sd64n4qa6k6m45/jemalloc-3.4.0-1.1.x86_64.rpm -O jemalloc.rpm
sudo rpm -i jemalloc.rpm
rm jemalloc.rpm
sudo ln -s /usr/lib64/libjemalloc.so.1 /usr/lib64/libjemalloc.so

git clone https://github.com/rmind/npf
cd npf/pkg
make rpm -j $(nproc)
sudo rpm -i ./RPMS/x86_64/npf-*.rpm
cd ~

wget https://osu.eu-west-2.outscale.com/jerome.jutteau/89esf1wd698h6sd64n4qa6k6m45/userspace-rcu-0.8.1-5.fc21.x86_64.rpm -O userspace-rcu.rpm
wget https://osu.eu-west-2.outscale.com/jerome.jutteau/89esf1wd698h6sd64n4qa6k6m45/userspace-rcu-devel-0.8.1-5.fc21.x86_64.rpm -O userspace-rcu-devel.rpm
sudo rpm -i userspace-rcu.rpm userspace-rcu-devel.rpm
rm userspace-rcu.rpm userspace-rcu-devel.rpm

echo "vm.nr_hugepages=1400" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p /etc/sysctl.conf
sudo mkdir -p /mnt/huge
echo "none    /mnt/huge       hugetlbfs        defaults        0       0" | sudo tee -a /etc/fstab
sudo mount -a

mkdir build
cd build
cmake ~/packetgraph
make
cd ~

# Actions below this point are used for running tests
sudo yum -i install ncurses-devel pixman-devel numactl-devel bc glibc.i686

git clone git://git.qemu-project.org/qemu.git
cd qemu
git submodule update --init dtc
./configure --enable-vnc --enable-vhost-net --enable-curses --enable-kvm --enable-numa
make -j $(nproc)
cd ~
sudo ln -s $HOME/qemu/x86_64-softmmu/qemu-system-x86_64 /usr/bin/

cd build
make tests
