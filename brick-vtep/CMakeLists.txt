cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME packetgraph-vtep)

project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(GLIB2 REQUIRED)
find_package(packetgraph-core REQUIRED)
find_package(packetgraph-hub REQUIRED)

set(RTE_SDK_ROOT $ENV{RTE_SDK})
if("${RTE_SDK_ROOT}" STREQUAL "")
	message(FATAL_ERROR "DPDK not found, is RTE_SDK global variable set ?")
endif()

set(CMAKE_C_FLAGS "-g -O3 -std=gnu11 -march=core-avx-i -mtune=core-avx-i -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -fPIC -D RTE_MAX_LCORE=128 -D RTE_PKTMBUF_HEADROOM=128")

add_library(${PROJECT_NAME}-objects OBJECT 
	./src/vtep.c)

target_include_directories(${PROJECT_NAME}-objects PUBLIC
	.
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

set(HEADERS
		vtep.h)

add_library(${PROJECT_NAME}-shared
		SHARED $<TARGET_OBJECTS:${PROJECT_NAME}-objects>)

add_library(${PROJECT_NAME}-static
		STATIC $<TARGET_OBJECTS:${PROJECT_NAME}-objects>)

set_target_properties(${PROJECT_NAME}-static ${PROJECT_NAME}-shared
		PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}-shared
	LIBRARY
	DESTINATION /usr/lib64)

install(TARGETS ${PROJECT_NAME}-static
	ARCHIVE
	DESTINATION /usr/lib64)

foreach(HEADER ${HEADERS})
	string(REGEX MATCH "(.*)[/\\]" DIR ${HEADER})
	install(FILES packetgraph/${HEADER}
		DESTINATION /usr/include/packetgraph/${DIR})
endforeach(HEADER ${HEADERS})

file(STRINGS ${CMAKE_SOURCE_DIR}/cmake/pkg/version.txt VERSION)
file(STRINGS ${CMAKE_SOURCE_DIR}/cmake/pkg/release.txt RELEASE)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_MAINTAINER "Matthias Gatto")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/cmake/pkg/description.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "packetgraph library permits to interconnect network bricks and create a dynamic software defined network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_RELEASE ${RELEASE})
set(CPACK_PACKAGE_CONTACT "contact@outscale.com")
set(CPACK_PACKAGE_VENDOR "Outscale SAS")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
# RPM specific
set(CPACK_RPM_PACKAGE_URL "https://gitdiode.com/outscale/butterfly")
set(CPACK_RPM_PACKAGE_LICENSE "GPL")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/cmake/pkg/changelog.txt")
set(CPACK_RPM_PACKAGE_REQUIRES "glib2 >= 2.40.0, packetgraph-core >= 0.0.1")
set(CPACK_GENERATOR "RPM")
include(CPack)

# Tests

add_executable(${PROJECT_NAME}-tests
		tests/tests.c)

target_include_directories(${PROJECT_NAME}-tests PUBLIC
	.
	./tests
	${RTE_SDK_ROOT}/build/include
	${RTE_SDK_ROOT}/lib
	${GLIB2_INCLUDE_DIR}
	${GLIB2_INTERNAL_INCLUDE_DIR}
	${LIBPACKETGRAPH-HUB_INCLUDE_DIR}
	${LIBPACKETGRAPH-CORE_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}-tests
	${PROJECT_NAME}-shared
	${LIBPACKETGRAPH-CORE_LIBRARY}
	${LIBPACKETGRAPH-HUB_LIBRARY}
	${PCAP_LIBRARY}
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
		COMMAND ${PROJECT_NAME}-tests -c1 -n1 --socket-mem 64
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
