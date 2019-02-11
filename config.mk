#####################################
### Configuration for packetgraph ###
#####################################
srcdir = ./
OS = Linux
CC = gcc
PG_ASAN_CFLAGS = -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-sanitize=alignment --param asan-globals=0
PG_NAME = libpacketgraph
PG_dev_NAME = libpacketgraph-dev
GLIB_HEADERS = -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
GLIB_LIBS = -lglib-2.0
RTE_SDK_HEADERS = -I/home/axel/dpdk/build/include/
RTE_SDK_LIBS = -lm -lpthread -ldl -lpcap -lnuma -L/home/axel/dpdk/build/lib/ -ldpdk
