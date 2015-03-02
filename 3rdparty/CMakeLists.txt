set(DPDK_SOURCE_DIR ${3RDPARTY_DIR}/dpdk)
set(BUILDROOT_SOURCE_DIR ${3RDPARTY_DIR}/buildroot)

set(CMAKE_C_FLAGS "-g -Wall")

# build targets
add_custom_target(dpdk ALL
	DEPENDS ${DPDK_SOURCE_DIR}/build/build/app/_postbuild)

add_custom_target(rumprun-posix ALL
	DEPENDS ${3RDPARTY_DIR}/rumprun-posix/rumpdyn/lib/librumpnet_npf.so)

add_custom_target(ccan ALL
	DEPENDS ${3RDPARTY_DIR}/ccan/libccan.a)

# build commands
add_custom_command(
	OUTPUT ${DPDK_SOURCE_DIR}/build/build/app/_postbuild
	COMMAND make config T=x86_64-native-linuxapp-gcc
	COMMAND make CONFIG_RTE_LIBRTE_VHOST=y
	WORKING_DIRECTORY dpdk
	COMMENT Building DPDK)

add_custom_command(
	OUTPUT ${3RDPARTY_DIR}/rumprun-posix/rumpdyn/lib/librumpnet_npf.so
	COMMAND ./buildnb.sh
	COMMAND . ./rumpremote.sh
	WORKING_DIRECTORY rumprun-posix
	COMMENT Building rump kernel posix component)

add_custom_command(
	OUTPUT ${3RDPARTY_DIR}/ccan/libccan.a
	COMMAND make
	WORKING_DIRECTORY ccan
	COMMENT Building CCAN)

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

# install targets
add_custom_target (dpdk-install ALL
	DEPENDS ${DPDK_INSTALL_DIR}/include/rte_config.h)
add_dependencies(dpdk-install dpdk)

add_custom_target (rumprun-posix-install ALL
	DEPENDS ${RUMPRUN_POSIX_INSTALL_DIR}/include/rump/rump.h)
add_dependencies(rumprun-posix-install rumprun-posix)

add_custom_target (ccan-install ALL
	DEPENDS  ${CCAN_INSTALL_DIR}/lib/libccan.a)
add_dependencies(ccan-install ccan)

# install commands
add_custom_command(
	OUTPUT ${DPDK_INSTALL_DIR}/include/rte_config.h
	COMMAND make install CONFIG_RTE_LIBRTE_VHOST=y T=x86_64-native-linuxapp-gcc DESTDIR=${3RDPARTY_INSTALL_DIR}
	WORKING_DIRECTORY dpdk
	COMMENT Installing DPDK)

add_custom_command(
	OUTPUT ${RUMPRUN_POSIX_INSTALL_DIR}/include/rump/rump.h
	COMMAND mkdir -p  ${RUMPRUN_POSIX_INSTALL_DIR}
	COMMAND cp -a rumpdyn/* ${RUMPRUN_POSIX_INSTALL_DIR}
	COMMAND cp -a bin ${RUMPRUN_POSIX_INSTALL_DIR}
	WORKING_DIRECTORY rumprun-posix
	COMMENT Installing rump kernel posix component)

add_custom_command(
	OUTPUT ${CCAN_INSTALL_DIR}/lib/libccan.a
	COMMAND mkdir -p ${CCAN_INSTALL_DIR}/lib
	COMMAND mkdir -p ${CCAN_INSTALL_DIR}/include
	COMMAND cp -a libccan.a ${CCAN_INSTALL_DIR}/lib
	COMMAND cp -a config.h  ${CCAN_INSTALL_DIR}/include
	COMMAND find . -name '*.h'|grep '^./ccan'|tar -c  --files-from=-|tar -x  -C ${CCAN_INSTALL_DIR}/include
	WORKING_DIRECTORY ccan
	COMMENT Installing CCAN)

# clean target
add_custom_target(dependency-cleanup CLEAN
	rm -rf ${3RDPARTY_INSTALL_DIR}
	COMMENT Deleting dependencies)

add_custom_target(3rdparty)
add_dependencies(3rdparty dpdk-install rumprun-posix-install ccan-install)

