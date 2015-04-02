cmake_minimum_required(VERSION 2.6)
include(ExternalProject)
project(BUTTERFLY)

# 3rdparties directories
set(3RDPARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

set(3RDPARTY_INSTALL_DIR ${PROJECT_BINARY_DIR}/3rdparty-build)
set(DPDK_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/x86_64-native-linuxapp-gcc)
set(RUMPRUN_POSIX_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/rumprun_posix)
set(CCAN_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/ccan)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

find_package(GLIB2 REQUIRED)
find_package(LibURCU REQUIRED)

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes")

include_directories(${GLIB2_INCLUDE_DIR} ${GLIB2_INTERNAL_INCLUDE_DIR}
		    ${LIBURCU_INCLUDE_DIR}
		    ${PROJECT_SOURCE_DIR}/packetgraph/include
		    ${DPDK_INSTALL_DIR}/include
		    ${CCAN_INSTALL_DIR}/include)

link_directories(${GLIB2_LIBRARIES}
		 ${DPDK_INSTALL_DIR}/lib
		 ${LIBURCU_LIBRARY})

set(TARGET_LIBRARIES ${GLIB2_LIBRARIES}
		     pthread
		     dl
		     urcu
		     ${CCAN_INSTALL_DIR}/lib/libccan.a
		     ${DPDK_INSTALL_DIR}/lib/librte_hash.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mbuf.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mempool.a
		     ${DPDK_INSTALL_DIR}/lib/librte_table.a
		     ${DPDK_INSTALL_DIR}/lib/librte_ring.a
		     ${DPDK_INSTALL_DIR}/lib/librte_eal.a
		     ${DPDK_INSTALL_DIR}/lib/librte_malloc.a
		     ${DPDK_INSTALL_DIR}/lib/librte_vhost.a)

set(CPPCHECK_COMMAND cppcheck -q  -f
	-I ${PROJECT_SOURCE_DIR}/packetgraph/include
	--enable=warning
	--enable=style
	--enable=performance
	--enable=portability
	--enable=information
	--enable=missingInclude)

add_subdirectory(3rdparty)
add_subdirectory(packetgraph)

set(DIRECTORIES packetgraph/bricks/ packetgraph/include/
		packetgraph/packets/ packetgraph/utils/)

add_custom_target(lizard
	COMMAND ${PROJECT_SOURCE_DIR}/3rdparty/lizard/lizard -C 10 -w  ${DIRECTORIES})

add_custom_target(flawfinder
	COMMAND flawfinder -D --quiet ${DIRECTORIES})
add_dependencies(flawfinder lizard)

add_custom_target(rats tests
	COMMAND rats --quiet ${DIRECTORIES})
add_dependencies(rats flawfinder)

# Add custom targets for tests

add_custom_target(tests-all
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_all.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

add_custom_target(tests-packetgraph
                  COMMAND ${PROJECT_SOURCE_DIR}/scripts/tests_packetgraph.sh ${PROJECT_SOURCE_DIR}
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
