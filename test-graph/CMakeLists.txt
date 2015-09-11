cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME test-graph)

project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../tools/cmake")
find_package(GLIB2 REQUIRED)
find_package(NPF REQUIRED)
find_package(NPFKERN REQUIRED)
find_package(LibURCU REQUIRED)
find_package(packetgraph-antispoof REQUIRED)
find_package(packetgraph-core REQUIRED)
find_package(packetgraph-vtep REQUIRED)
find_package(packetgraph-vhost REQUIRED)
find_package(packetgraph-nic REQUIRED)
find_package(packetgraph-print REQUIRED)
find_package(packetgraph-firewall REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC")


add_executable(${PROJECT_NAME}
		test-graph.c)

target_include_directories(${PROJECT_NAME} PUBLIC
	.
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR}
	${LIBPACKETGRAPH-NIC_INCLUDE_DIR}
	${LIBPACKETGRAPH-PRINT_INCLUDE_DIR}
	${LIBPACKETGRAPH-VTEP_INCLUDE_DIR}	
	${LIBPACKETGRAPH-NIC_INCLUDE_DIR}
	${LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR}
	${LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR}
	${LIBPACKETGRAPH-VHOST_INCLUDE_DIR}
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
	${LIBPACKETGRAPH-NIC_LIBRARY}
	${LIBPACKETGRAPH-VTEP_LIBRARY}	
	${LIBPACKETGRAPH-VHOST_LIBRARY}	
	${LIBPACKETGRAPH-PRINT_LIBRARY}
	${LIBPACKETGRAPH-NIC_LIBRARY}
	${LIBPACKETGRAPH-FIREWALL_LIBRARY}
	${LIBPACKETGRAPH-ANTISPOOF_LIBRARY}
	${LIBPACKETGRAPH-CORE_LIBRARY}	
	${LIBNPF_LIBRARY}
	${LIBNPFKERN_LIBRARY}
	${LIBURCU_LIBRARY}
	pthread
	dl
	urcu
	z
	cdb
	pcap
	dl
	${GLIB2_LIBRARIES}
	${RTE_SDK_ROOT}/build/lib/librte_vhost.a
	${RTE_SDK_ROOT}/build/lib/librte_hash.a
	${RTE_SDK_ROOT}/build/lib/librte_mbuf.a
	${RTE_SDK_ROOT}/build/lib/librte_mempool.a
	${RTE_SDK_ROOT}/build/lib/librte_table.a
	${RTE_SDK_ROOT}/build/lib/librte_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_eal.a
	${RTE_SDK_ROOT}/build/lib/librte_malloc.a
	${RTE_SDK_ROOT}/build/lib/libethdev.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_pcap.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_e1000.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ixgbe.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)

add_custom_target(tests
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests.sh ${LIBPACKETGRAPH-VHOST_INCLUDE_DIR} ${PG_USER_VHOST_BUILD}  ${CMAKE_BINARY_DIR}
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
