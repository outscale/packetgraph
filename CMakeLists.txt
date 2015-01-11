cmake_minimum_required(VERSION 2.6)
project(BUTTERFLY)

# 3rdparties directories
set(3RDPARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

set(3RDPARTY_INSTALL_DIR ${PROJECT_SOURCE_DIR}/3rdparty-build)
set(DPDK_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/x86_64-native-linuxapp-gcc)
set(RUMPRUN_POSIX_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/rumprun_posix)
set(CCAN_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/ccan)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

find_package(GLIB2 REQUIRED)
find_package(ProtobufC REQUIRED)

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wcast-qual -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes ${PROTOBUFC_DEFINITIONS}")

include_directories(${GLIB2_INCLUDE_DIR} ${GLIB2_INTERNAL_INCLUDE_DIR}
		    ${PROTOBUFC_INCLUDE_DIRS}
		    ${PROJECT_SOURCE_DIR}/include
		    ${DPDK_INSTALL_DIR}/include
		    ${CCAN_INSTALL_DIR}/include
		    ${PROJECT_SOURCE_DIR}/protos)

link_directories(${GLIB2_LIBRARIES}
		 ${DPDK_INSTALL_DIR}/lib
		 ${PROTOBUFC_LIBRARIES})

set(TARGET_LIBRARIES ${GLIB2_LIBRARIES}
		     pthread
		     dl
		     ${CCAN_INSTALL_DIR}/lib/libccan.a
		     ${DPDK_INSTALL_DIR}/lib/librte_hash.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mbuf.a
		     ${DPDK_INSTALL_DIR}/lib/librte_mempool.a
		     ${DPDK_INSTALL_DIR}/lib/librte_table.a
		     ${DPDK_INSTALL_DIR}/lib/librte_ring.a
		     ${DPDK_INSTALL_DIR}/lib/librte_eal.a
		     ${DPDK_INSTALL_DIR}/lib/librte_malloc.a
		     ${PROTOBUFC_LIBRARIES})

set(CPPCHECK_COMMAND cppcheck -q  -f
	-I ${PROJECT_SOURCE_DIR}/include
	-I ${PROJECT_SOURCE_DIR}/protos
	--enable=warning
	--enable=style
	--enable=performance
	--enable=portability
	--enable=information
	--enable=missingInclude)

add_subdirectory(3rdparty)
add_subdirectory(utils)
add_subdirectory(packets)
add_subdirectory(protos)
add_subdirectory(bricks)
add_subdirectory(tests)

set(DIRECTORIES bricks/ include/ packets/ utils/)

add_custom_target(flawfinder
	COMMAND flawfinder -D --quiet ${DIRECTORIES})
add_dependencies(flawfinder tests)

add_custom_target(rats
	COMMAND rats --quiet ${DIRECTORIES})
add_dependencies(rats flawfinder)
