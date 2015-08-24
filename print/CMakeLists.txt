cmake_minimum_required(VERSION 2.6)

project(packetgraph-print)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../tools/cmake")
find_package(GLIB2 REQUIRED)
find_package(packetgraph-core REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -std=gnu11")

add_library(packetgraph-print-objects OBJECT 
	./src/print.c
	./src/printer.c)

target_include_directories(packetgraph-print-objects PUBLIC
	./src
	.
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

set(HEADERS
		print.h)

add_library(packetgraph-print-shared
		SHARED $<TARGET_OBJECTS:packetgraph-print-objects>)

add_library(packetgraph-print-static
		STATIC $<TARGET_OBJECTS:packetgraph-print-objects>)

set_target_properties(packetgraph-print-static packetgraph-print-shared
		PROPERTIES OUTPUT_NAME packetgraph-print)

install(TARGETS packetgraph-print-shared
	LIBRARY
	DESTINATION /usr/lib64)

install(TARGETS packetgraph-print-static
	ARCHIVE
	DESTINATION /usr/lib64)

foreach(HEADER ${HEADERS})
	string(REGEX MATCH "(.*)[/\\]" DIR ${HEADER})
	install(FILES packetgraph/${HEADER}
		DESTINATION /usr/include/packetgraph/${DIR})
endforeach(HEADER ${HEADERS})

set(PKG ${CMAKE_SOURCE_DIR}/pkg)
file(STRINGS ${PKG}/version.txt VERSION)
file(STRINGS ${PKG}/release.txt RELEASE)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_MAINTAINER "Jerome Jutteau")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PKG}/description.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "packetgraph library permits to interconnect network bricks and create a dynamic software defined network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_RELEASE ${RELEASE})
set(CPACK_PACKAGE_CONTACT "contact@outscale.com")
set(CPACK_PACKAGE_VENDOR "Outscale SAS")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
# RPM specific
set(CPACK_RPM_PACKAGE_URL "https://gitantispoof.com/outscale/packetgraph")
set(CPACK_RPM_PACKAGE_LICENSE "GPL")
set(CPACK_RPM_CHANGELOG_FILE "${PKG}/changelog.txt")
set(CPACK_RPM_PACKAGE_REQUIRES "glib2 >= 2.40.0, packetgraph-core >= 0.0.1")
set(CPACK_GENERATOR "RPM")
include(CPack)

# Tests

add_executable(packetgraph-print-tests 
		tests/tests.c)

target_include_directories(packetgraph-print-tests PUBLIC
	.
	./tests
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

target_link_libraries(packetgraph-print-tests
	packetgraph-print-shared
	${LIBPACKETGRAPH-CORE_LIBRARY}
	pthread
	dl
	${GLIB2_LIBRARIES}
	${RTE_SDK_ROOT}/build/lib/librte_hash.a
	${RTE_SDK_ROOT}/build/lib/librte_mbuf.a
	${RTE_SDK_ROOT}/build/lib/librte_mempool.a
	${RTE_SDK_ROOT}/build/lib/librte_table.a
	${RTE_SDK_ROOT}/build/lib/librte_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_eal.a
	${RTE_SDK_ROOT}/build/lib/librte_malloc.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)

add_custom_target(tests
		COMMAND packetgraph-print-tests -c1 -n1 --socket-mem 64
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
