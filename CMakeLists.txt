cmake_minimum_required(VERSION 2.6)
include(ExternalProject)
project(BUTTERFLY)

# 3rdparties directories
set(3RDPARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

set(3RDPARTY_INSTALL_DIR ${PROJECT_BINARY_DIR}/3rdparty-build)
set(DPDK_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/x86_64-native-linuxapp-gcc)
set(RUMPRUN_POSIX_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/rumprun_posix)
set(CCAN_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/ccan)
set(LIBZMQ_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/libzmq)
set(ZMQPP_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/zmqpp)
set(PROTOBUF_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/protobuf)
set(NPF_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/npf)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

find_package(GLIB2 REQUIRED)
find_package(LibURCU REQUIRED)

set(PROTOBUF_LIBRARIES ${PROTOBUF_INSTALL_DIR}/src/.libs/libprotobuf-lite.a
                       ${PROTOBUF_INSTALL_DIR}/src/.libs/libprotobuf.a)
set(PROTOBUF_INCLUDE_DIR ${3RDPARTY_DIR}/protobuf/src)
set(PROTOBUF_PROTOC_EXECUTABLE ${PROTOBUF_INSTALL_DIR}/src/protoc)
set(LIBZMQ_LIBRARIES ${LIBZMQ_INSTALL_DIR}/lib/libzmq-static.a)
set(LIBZMQ_INCLUDE_DIR ${3RDPARTY_DIR}/libzmq/include)
set(ZMQPP_LIBRARIES ${ZMQPP_INSTALL_DIR}/libzmqpp-static.a)
set(ZMQPP_INCLUDE_DIR ${3RDPARTY_DIR}/zmqpp/src)

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes")

include_directories(${GLIB2_INCLUDE_DIR} ${GLIB2_INTERNAL_INCLUDE_DIR}
		    ${LIBURCU_INCLUDE_DIR}
		    ${PROJECT_SOURCE_DIR}/packetgraph/include
		    ${DPDK_INSTALL_DIR}/include
		    ${CCAN_INSTALL_DIR}/include
		    ${3RDPARTY_INSTALL_DIR}
		    ${NPF_INSTALL_DIR}/dpdk
		    ${NPF_INSTALL_DIR}/src/libnpf
		    ${NPF_INSTALL_DIR}/src/kern)

link_directories(${GLIB2_LIBRARIES}
		 ${DPDK_INSTALL_DIR}/lib
		 ${LIBURCU_LIBRARY})

set(TARGET_LIBRARIES ${GLIB2_LIBRARIES}
		     pthread
		     dl
		     urcu
		     z
		     cdb
		     npf
		     npfkern
		     pcap
		     ${CCAN_INSTALL_DIR}/lib/libccan.a
		     ${DPDK_INSTALL_DIR}/lib/libethdev.a
		     ${DPDK_INSTALL_DIR}/lib/librte_hash.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mbuf.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mempool.a
		     ${DPDK_INSTALL_DIR}/lib/librte_table.a
		     ${DPDK_INSTALL_DIR}/lib/librte_ring.a
		     ${DPDK_INSTALL_DIR}/lib/librte_eal.a
		     ${DPDK_INSTALL_DIR}/lib/librte_malloc.a
		     ${DPDK_INSTALL_DIR}/lib/librte_vhost.a
		     ${DPDK_INSTALL_DIR}/lib/librte_pmd_pcap.a
		     ${DPDK_INSTALL_DIR}/lib/librte_pmd_ring.a
		     ${DPDK_INSTALL_DIR}/lib/librte_kvargs.a
		     ${DPDK_INSTALL_DIR}/lib/librte_pmd_e1000.a
		     ${DPDK_INSTALL_DIR}/lib/librte_pmd_ixgbe.a
		     ${NPF_INSTALL_DIR}/dpdk/build/npf_dpdk.o
             )

add_subdirectory(3rdparty)
add_subdirectory(packetgraph)
add_subdirectory(api)

set(DIRECTORIES ${PROJECT_SOURCE_DIR}/packetgraph/bricks/ ${PROJECT_SOURCE_DIR}/packetgraph/include/
		${PROJECT_SOURCE_DIR}/packetgraph/packets/ ${PROJECT_SOURCE_DIR}/packetgraph/utils/)

add_custom_target(lizard-run
	COMMAND ${PROJECT_SOURCE_DIR}/3rdparty/lizard/lizard -C 10 -w  ${DIRECTORIES})

add_custom_target(flawfinder
	COMMAND flawfinder -D --quiet ${DIRECTORIES})

add_custom_target(rats
	COMMAND rats  ${DIRECTORIES})

# Add custom targets for tests

add_custom_target(tests-all
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_all.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(tests-packetgraph
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_packetgraph.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(tests-packetgraph-style
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_packetgraph_style.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(tests-api-style
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_api_style.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(tests-api
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_api.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
