set(DPDK_SOURCE_DIR ${3RDPARTY_DIR}/dpdk)
set(BUILDROOT_SOURCE_DIR ${3RDPARTY_DIR}/buildroot)
set(CCAN_SOURCE_DIR ${3RDPARTY_DIR}/ccan)
set(LIZARD_SOURCE_DIR ${3RDPARTY_DIR}/lizard)

set(CMAKE_C_FLAGS "-g -Wall")

add_custom_command(
	OUTPUT ${CCAN_INSTALL_DIR}/lib/libccan.a
	COMMAND mkdir -p ${CCAN_INSTALL_DIR}/lib
	COMMAND mkdir -p ${CCAN_INSTALL_DIR}/include
	COMMAND cp -a libccan.a ${CCAN_INSTALL_DIR}/lib
	COMMAND cp -a config.h  ${CCAN_INSTALL_DIR}/include
	COMMAND find . -name '*.h'|grep '^./ccan'|tar -c  --files-from=-|tar -x  -C ${CCAN_INSTALL_DIR}/include
	WORKING_DIRECTORY ${CCAN_SOURCE_DIR}
	COMMENT Installing CCAN)

ExternalProject_Add(ccan
	SOURCE_DIR ${CCAN_SOURCE_DIR}
	CONFIGURE_COMMAND true
	DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}
	DOWNLOAD_COMMAND git submodule update --init ${CCAN_SOURCE_DIR}
	BINARY_DIR ${CCAN_SOURCE_DIR}
	BUILD_COMMAND make
	INSTALL_DIR ${CCAN_SOURCE_DIR}
	INSTALL_COMMAND true
	LOG_BUILD 1 Building buildroot)

add_custom_target (ccan-install ALL
	DEPENDS  ${CCAN_INSTALL_DIR}/lib/libccan.a)

ExternalProject_Add(dpdk
	SOURCE_DIR ${DPDK_SOURCE_DIR}
	DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}
	DOWNLOAD_COMMAND git submodule update --init ${DPDK_SOURCE_DIR}
	CONFIGURE_COMMAND make config T=x86_64-native-linuxapp-gcc CONFIG_RTE_LIBRTE_VHOST=y
	BINARY_DIR ${DPDK_SOURCE_DIR}
	BUILD_COMMAND make
	INSTALL_DIR ${DPDK_INSTALL_DIR}
	INSTALL_COMMAND make install CONFIG_RTE_LIBRTE_VHOST=y T=x86_64-native-linuxapp-gcc DESTDIR=${3RDPARTY_INSTALL_DIR}
	LOG_BUILD 1 Building dpdk)

ExternalProject_Add(buildroot
	PATCH_COMMAND patch -t -N -p1 < ${3RDPARTY_DIR}/patches/0001-config-custom-buildroot-config-for-testing.patch || true
	DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}
	DOWNLOAD_COMMAND git submodule update --init ${BUILDROOT_SOURCE_DIR}
	SOURCE_DIR ${BUILDROOT_SOURCE_DIR}
	UPDATE_COMMAND chmod +x overlay/sbin/init
	CONFIGURE_COMMAND make oldconfig
	BINARY_DIR ${BUILDROOT_SOURCE_DIR}
	BUILD_COMMAND make
	INSTALL_COMMAND true
	LOG_BUILD 1 Building buildroot)

ExternalProject_Add(lizard
	SOURCE_DIR ${LIZARD_SOURCE_DIR}
	DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}
	DOWNLOAD_COMMAND git submodule update --init ${LIZARD_SOURCE_DIR}
	CONFIGURE_COMMAND ""
	BINARY_DIR ${LIZARD_SOURCE_DIR}
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	LOG_BUILD 1 Building lizard)

# clean target
add_custom_target(dependency-cleanup CLEAN
	rm -rf ${3RDPARTY_INSTALL_DIR}
	COMMENT Deleting dependencies)

add_custom_target(3rdparty)
add_dependencies(3rdparty ccan-install)

