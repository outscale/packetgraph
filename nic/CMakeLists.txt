set(BRICK nic)
project(packetgraph-${BRICK})

# Check if we build from packetgraph root or not
set(PG_ROOT ${PG_ROOT_PATH})
if("${PG_ROOT_PATH}" STREQUAL "")
    cmake_minimum_required(VERSION 2.6)
    set(PG_ROOT ${CMAKE_SOURCE_DIR}/..)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PG_ROOT}/tools/cmake")
find_package(GLIB2 REQUIRED)
find_package(packetgraph-core REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC")

add_library(packetgraph-${BRICK}-objects OBJECT 
	${PG_ROOT}/${BRICK}/src/nic.c)

target_include_directories(packetgraph-${BRICK}-objects PUBLIC
	${PG_ROOT}/${BRICK}
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

set(HEADERS
		nic.h)

add_library(packetgraph-${BRICK}-shared
		SHARED $<TARGET_OBJECTS:packetgraph-${BRICK}-objects>)

add_library(packetgraph-${BRICK}-static
		STATIC $<TARGET_OBJECTS:packetgraph-${BRICK}-objects>)

set_target_properties(packetgraph-${BRICK}-static packetgraph-${BRICK}-shared
		PROPERTIES OUTPUT_NAME packetgraph-${BRICK})

install(TARGETS packetgraph-${BRICK}-shared
	LIBRARY
	DESTINATION /usr/lib64)

install(TARGETS packetgraph-${BRICK}-static
	ARCHIVE
	DESTINATION /usr/lib64)

foreach(HEADER ${HEADERS})
	string(REGEX MATCH "(.*)[/\\]" DIR ${HEADER})
	install(FILES ${PG_ROOT}/${BRICK}/packetgraph/${HEADER}
		DESTINATION /usr/include/packetgraph/${DIR})
endforeach(HEADER ${HEADERS})

set(PKG ${PG_ROOT}/${BRICK}/pkg)
file(STRINGS ${PKG}/version.txt VERSION)
file(STRINGS ${PKG}/release.txt RELEASE)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_MAINTAINER "Jerome Jutteau")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PKG}/description.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "packetgraph library permits to interconnect network bricks and create a dynamic software defined network")
set(CPACK_RESOURCE_FILE_LICENSE "${PG_ROOT}/${BRICK}/LICENSE")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_RELEASE ${RELEASE})
set(CPACK_PACKAGE_CONTACT "contact@outscale.com")
set(CPACK_PACKAGE_VENDOR "Outscale SAS")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
# RPM specific
set(CPACK_RPM_PACKAGE_URL "https://github.com/outscale/packetgraph")
set(CPACK_RPM_PACKAGE_LICENSE "GPL")
set(CPACK_RPM_CHANGELOG_FILE "${PKG}/changelog.txt")
set(CPACK_RPM_PACKAGE_REQUIRES "glib2 >= 2.40.0, packetgraph-core >= 0.0.1")
set(CPACK_GENERATOR "RPM")
include(CPack)

# Tests

# We recompile nic.c with _BRICKS_BRICK_NIC_STUB_H_ in order
# to test even without a real NIC.

add_executable(packetgraph-${BRICK}-tests EXCLUDE_FROM_ALL
		src/nic.c
		tests/test-nic.c
		tests/tests.c)

set_target_properties(packetgraph-${BRICK}-tests
		PROPERTIES COMPILE_FLAGS "-D _BRICKS_BRICK_NIC_STUB_H_")

target_include_directories(packetgraph-${BRICK}-tests PUBLIC
	${PG_ROOT}/${BRICK}
	${PG_ROOT}/${BRICK}/tests
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

target_link_libraries(packetgraph-${BRICK}-tests
	${LIBPACKETGRAPH-CORE_LIBRARY}
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
	${RTE_SDK_ROOT}/build/lib/libethdev.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_pcap.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_e1000.a
	${RTE_SDK_ROOT}/build/lib/librte_pmd_ixgbe.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)

add_custom_target(packetgraph-${BRICK}-tests-run
		COMMAND /usr/bin/sudo ${PROJECT_BINARY_DIR}/packetgraph-${BRICK}-tests -c1 -n1 --socket-mem 64 --vdev=eth_pcap0,rx_pcap=${CMAKE_CURRENT_SOURCE_DIR}/tests/enter.pcap,tx_pcap=out.pcap --vdev=eth_ring0)
add_dependencies(packetgraph-${BRICK}-tests-run packetgraph-${BRICK}-static)
add_dependencies(packetgraph-${BRICK}-tests-run packetgraph-${BRICK}-tests)
if (NOT "${PG_ROOT_PATH}" STREQUAL "")
    add_dependencies(packetgraph-${BRICK}-tests packetgraph-core-static)
endif()

