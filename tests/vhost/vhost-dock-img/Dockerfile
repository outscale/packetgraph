FROM ubuntu:latest

LABEL RUN docker run -it --privileged -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev --name NAME -e NAME=NAME -e IMAGE=IMAGE IMAGE

RUN apt-get update && \
      apt-get -y install sudo

WORKDIR /usr/src/dpdk
COPY . /usr/src/dpdk
COPY . /usr/bin/
RUN apt-get update
RUN apt-get -y install numactl libpcap-dev libglib2.0-dev libjemalloc-dev
RUN ln -s /usr/lib/x86_64-linux-gnu/libpcap.so.1.* /usr/lib/x86_64-linux-gnu/libpcap.so.1
RUN ln -s /usr/lib/x86_64-linux-gnu/libjemalloc.so.1 /usr/lib/x86_64-linux-gnu/libjemalloc.so.2
ENV LD_LIBRARY_PATH "/usr/lib/x86_64-linux-gnu/:/usr/src/dpdk/pg-libs/"
ENV PATH "/bin/:/usr/bin/:/sbin/:/usr/sbin/:/usr/src/dpdk/x86_64-native-linuxapp-gcc/app/:/usr/src/dpdk/pktgen-3.4.9/app/x86_64-native-linuxapp-gcc/:/usr/src/dpdk/pg-libs/"
