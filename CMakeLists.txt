cmake_minimum_required(VERSION 2.6)
project(BUTTERFLY)

# 3rdparties directories
set(3RDPARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

set(3RDPARTY_INSTALL_DIR ${PROJECT_SOURCE_DIR}/3rdparty-build)
set(DPDK_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/x86_64-native-linuxapp-gcc)
set(RUMPRUN_POSIX_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/rumprun_posix)
set(CCAN_INSTALL_DIR ${3RDPARTY_INSTALL_DIR}/ccan)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules/")

include_directories(${PROJECT_SOURCE_DIR}/include
		    ${DPDK_INSTALL_DIR}/include
		    ${CCAN_INSTALL_DIR}/include)

add_subdirectory(3rdparty)
