#!/bin/bash
set -x
set -e
cd ~
git clone http://dpdk.org/git/dpdk
cd dpdk
git checkout -b v18.08 v18.08
export RTE_SDK=~/dpdk
make config T=x86_64-native-linuxapp-clang
sed -i -e "s/CONFIG_RTE_LIBRTE_PMD_PCAP=n/CONFIG_RTE_LIBRTE_PMD_PCAP=y/" build/.config
make EXTRA_CFLAGS='-fPIC'

cd $TRAVIS_BUILD_DIR
git config user.name "${GH_USER_NAME}"
git config user.email "${USER_MAIL}"
./autogen.sh
export LD="llvm-link"
./configure_clang --with-benchmarks --with-examples

make

make tests-antispoof
make tests-core
make tests-diode
make tests-firewall
make tests-integration
make tests-nic
make tests-print
make tests-queue
make tests-switch
make tests-vhost
make tests-vtep
make tests-rxtx
make tests-pmtud
make tests-tap
make tests-ip-fragment
make tests-thread

if [ ! -e "/usr/bin/doxygen" ]; then
    sudo curl https://osu.eu-west-2.outscale.com/jerome.jutteau/16d1bc0517de5c95aa076a0584b43af6/doxygen.sh -o /usr/bin/doxygen
    sudo chmod +x /usr/bin/doxygen
fi
