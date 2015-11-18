cmake_minimum_required(VERSION 2.6)

project(example-dperf)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/cmake")
find_package(GLIB2 REQUIRED)
find_package(PCAP REQUIRED)
find_package(packetgraph-core REQUIRED)
find_package(packetgraph-nic REQUIRED)
find_package(packetgraph-vhost REQUIRED)
find_package(packetgraph-print REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -D RTE_MAX_LCORE=128 -D RTE_PKTMBUF_HEADROOM=128 -std=gnu11")


set(SOURCES dperf.c)

add_executable(dperf
  dperf.c)

target_include_directories(dperf PUBLIC
  .
  ${RTE_SDK_ROOT}/build/include
  ${RTE_SDK_ROOT}/lib
  ${GLIB2_INCLUDE_DIR}
  ${GLIB2_INTERNAL_INCLUDE_DIR}
  ${PCAP_INCLUDE_DIR}
  ${LIBNPF_INCLUDE_DIR}
  ${LIBPACKETGRAPH-CORE_INCLUDE_DIR}
  ${LIBPACKETGRAPH-NIC_INCLUDE_DIR}
  ${LIBPACKETGRAPH-VHOST_INCLUDE_DIR}
  ${LIBPACKETGRAPH-PRINT_INCLUDE_DIR})

target_link_libraries(dperf
	${LIBPACKETGRAPH-NIC_LIBRARY}
	${LIBPACKETGRAPH-VHOST_LIBRARY}
	${LIBPACKETGRAPH-PRINT_LIBRARY}
	${LIBPACKETGRAPH-CORE_LIBRARY}
	${LIBNPF_LIBRARY}
	${LIBNPFKERN_LIBRARY}
	${PCAP_LIBRARY}
	pthread
	dl
	urcu
	z
	cdb
	pcap
	${GLIB2_LIBRARIES}
	${RTE_SDK_ROOT}/build/lib/librte_hash.a
	${RTE_SDK_ROOT}/build/lib/librte_mbuf.a
	${RTE_SDK_ROOT}/build/lib/librte_mempool.a
	${RTE_SDK_ROOT}/build/lib/librte_table.a
	${RTE_SDK_ROOT}/build/lib/librte_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_eal.a
	${RTE_SDK_ROOT}/build/lib/librte_vhost.a
	${RTE_SDK_ROOT}/build/lib/libethdev.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_pcap.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_e1000.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ixgbe.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)
