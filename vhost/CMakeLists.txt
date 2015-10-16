cmake_minimum_required(VERSION 2.6)

project(packetgraph-vhost)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../tools/cmake")
find_package(GLIB2 REQUIRED)
find_package(LibURCU REQUIRED)
find_package(packetgraph-core REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC")

add_library(packetgraph-vhost-objects OBJECT 
	./src/vhost.c)

target_include_directories(packetgraph-vhost-objects PUBLIC
	.
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR}
	${LIBURCU_INCLUDE_DIR})

set(HEADERS
		vhost.h)

add_library(packetgraph-vhost-shared
		SHARED $<TARGET_OBJECTS:packetgraph-vhost-objects>)

add_library(packetgraph-vhost-static
		STATIC $<TARGET_OBJECTS:packetgraph-vhost-objects>)

set_target_properties(packetgraph-vhost-static packetgraph-vhost-shared
		PROPERTIES OUTPUT_NAME packetgraph-vhost)

install(TARGETS packetgraph-vhost-shared
	LIBRARY
	DESTINATION /usr/lib64)

install(TARGETS packetgraph-vhost-static
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
set(CPACK_RPM_PACKAGE_REQUIRES "glib2 >= 2.40.0, userspace-rcu-devel >= 0.8.1, packetgraph-core >= 0.0.1")
set(CPACK_GENERATOR "RPM")
include(CPack)

# Tests

add_executable(packetgraph-vhost-tests EXCLUDE_FROM_ALL
		tests/test-vhost.c
		tests/tests.c)

target_include_directories(packetgraph-vhost-tests PUBLIC
	.
	./tests
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${LIBURCU_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

target_link_libraries(packetgraph-vhost-tests
	packetgraph-vhost-shared
	${LIBPACKETGRAPH-CORE_LIBRARY}
	pthread
	dl
	${GLIB2_LIBRARIES}
	${LIBURCU_LIBRARY}
	${RTE_SDK_ROOT}/build/lib/librte_hash.a
	${RTE_SDK_ROOT}/build/lib/librte_mbuf.a
	${RTE_SDK_ROOT}/build/lib/librte_mempool.a
	${RTE_SDK_ROOT}/build/lib/librte_table.a
	${RTE_SDK_ROOT}/build/lib/librte_ring.a
	${RTE_SDK_ROOT}/build/lib/librte_eal.a
	${RTE_SDK_ROOT}/build/lib/librte_vhost.a
	${RTE_SDK_ROOT}/build/lib/librte_kvargs.a)

add_custom_target(tests
		COMMAND ./test.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_dependencies(tests ${CMAKE_PROJECT_NAME}-static)
add_dependencies(tests ${CMAKE_PROJECT_NAME}-tests)
