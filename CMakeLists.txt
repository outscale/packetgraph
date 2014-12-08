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

set(CMAKE_C_FLAGS "-g -fmessage-length=0  -Werror -Wall -Wextra -Wwrite-strings -Winit-self -Wcast-align -Wcast-qual -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes ${PROTOBUFC_DEFINITIONS}")

include_directories(${GLIB2_INCLUDE_DIR} ${GLIB2_INTERNAL_INCLUDE_DIR}
		    ${PROTOBUFC_INCLUDE_DIRS}
		    ${PROJECT_SOURCE_DIR}/include
		    ${DPDK_INSTALL_DIR}/include
		    ${CCAN_INSTALL_DIR}/include
		    ${PROJECT_SOURCE_DIR}/protos)

link_directories(${GLIB2_LIBRARIES} ${PROTOBUFC_LIBRARIES})

set(TARGET_LIBRARIES ${GLIB2_LIBRARIES} ${CCAN_INSTALL_DIR}/lib/libccan.a ${PROTOBUFC_LIBRARIES})
add_subdirectory(3rdparty)
add_subdirectory(utils)
add_subdirectory(protos)
add_subdirectory(bricks)
add_subdirectory(tests)
