set(BRICK core)
project(packetgraph-${BRICK})

# Check if we build from packetgraph root or not
set(PG_ROOT ${PG_ROOT_PATH})
if("${PG_ROOT_PATH}" STREQUAL "")
    cmake_minimum_required(VERSION 2.6)
    set(PG_ROOT ${CMAKE_SOURCE_DIR}/..)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PG_ROOT}/tools/cmake")
find_package(GLIB2 REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11")

add_library(packetgraph-${BRICK}-objects OBJECT 
	${PG_ROOT}/${BRICK}/src/utils/bitmask.c
	${PG_ROOT}/${BRICK}/src/utils/config.c
	${PG_ROOT}/${BRICK}/src/utils/errors.c
	${PG_ROOT}/${BRICK}/src/utils/qemu.c
	${PG_ROOT}/${BRICK}/src/lifecycle.c
	${PG_ROOT}/${BRICK}/src/utils/mempool.c
	${PG_ROOT}/${BRICK}/src/utils/mac.c
	${PG_ROOT}/${BRICK}/src/bricks.c
	${PG_ROOT}/${BRICK}/src/packets.c
	${PG_ROOT}/${BRICK}/src/nop.c
	${PG_ROOT}/${BRICK}/src/collect.c
	${PG_ROOT}/${BRICK}/src/graph.c
	${PG_ROOT}/${BRICK}/src/packetsgen.c)

target_include_directories(packetgraph-${BRICK}-objects PUBLIC
	${PG_ROOT}/${BRICK}
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR})

set(HEADERS
		brick-int.h
		brick.h
		common.h
		packetgraph.h
		packets.h
		nop.h
		collect.h
		graph.h
		packetsgen.h
		utils/bitmask.h
		utils/config.h
		utils/errors.h
		utils/mempool.h
		utils/mac.h
		ccan/config.h
		ccan/build_assert/build_assert.h
		ccan/compiler/compiler.h)

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
set(CPACK_PACKAGE_NAME packetgraph-${BRICK})
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
set(CPACK_RPM_PACKAGE_REQUIRES "glib2 >= 2.40.0")
set(CPACK_GENERATOR "RPM")
include(CPack)

# Tests

add_executable(packetgraph-${BRICK}-tests EXCLUDE_FROM_ALL
		tests/test-core.c
		tests/test-graph.c
		tests/test-flow.c
		tests/test-error.c
		tests/test-pkts-count.c
		tests/tests.c)

target_include_directories(packetgraph-${BRICK}-tests PUBLIC
	${PG_ROOT}/${BRICK}
	${PG_ROOT}/${BRICK}/tests
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR})

target_link_libraries(packetgraph-${BRICK}-tests
	packetgraph-${BRICK}-shared
	pthread
	dl
	${GLIB2_LIBRARIES}
	${RTE_SDK_ROOT}/build/lib/librte_hash.a
	${RTE_SDK_ROOT}/build/lib/librte_mbuf.a
	${RTE_SDK_ROOT}/build/lib/librte_mempool.a
	${RTE_SDK_ROOT}/build/lib/librte_table.a
	${RTE_SDK_ROOT}/build/lib/librte_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_eal.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)

add_custom_target(packetgraph-${BRICK}-tests-run
		COMMAND /usr/bin/sudo ${PROJECT_BINARY_DIR}/packetgraph-${BRICK}-tests -c1 -n1 --socket-mem 64)
add_dependencies(packetgraph-${BRICK}-tests-run packetgraph-${BRICK}-static)
add_dependencies(packetgraph-${BRICK}-tests-run packetgraph-${BRICK}-tests)
