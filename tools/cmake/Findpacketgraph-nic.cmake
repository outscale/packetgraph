# Finds packetgraph-nic includes and library
# Defines:
#  LIBPACKETGRAPH-NIC_INCLUDE_DIR
#  LIBPACKETGRAPH-NIC_LIBRARY
#  LIBPACKETGRAPH-NIC_FOUND

if (NOT "${PG_NIC}" STREQUAL "")
	set(LIBPACKETGRAPH-NIC_INCLUDE_DIR ${PG_ROOT}/nic)
	set(LIBPACKETGRAPH-NIC_LIBRARY "${PG_USER_NIC_BUILD}/libpacketgraph-nic.a")
	set(LIBPACKETGRAPH-NIC_LIBRARY_SHARED "${PG_USER_NIC_BUILD}/libpacketgraph-nic.so")
	set(LIBPACKETGRAPH-NIC_FOUND 1)
    return()
endif()

if (NOT "${PG_ROOT_PATH}" STREQUAL "")
	set(LIBPACKETGRAPH-NIC_INCLUDE_DIR ${PG_ROOT}/nic)
	set(LIBPACKETGRAPH-NIC_LIBRARY "${CMAKE_BINARY_DIR}/nic/libpacketgraph-nic.a")
	set(LIBPACKETGRAPH-NIC_LIBRARY_SHARED "${CMAKE_BINARY_DIR}/libpacketgraph-nic.so")
	set(LIBPACKETGRAPH-NIC_FOUND 1)
    return()
endif()

message ("PG_NIC global variable not set, searching packetgraph-nic on the system ...")

find_path(LIBPACKETGRAPH-NIC_ROOT_DIR
	NAMES include/packetgraph/packetgraph.h
)

find_path(LIBPACKETGRAPH-NIC_INCLUDE_DIR
		NAMES packetgraph.h
		HINTS ${LIBPACKETGRAPH-NIC_ROOT_DIR}/include/packetgraph
)

set(LIBPACKETGRAPH-NIC_NAME packetgraph-nic)

find_library(LIBPACKETGRAPH-NIC_LIBRARY
		NAMES ${LIBPACKETGRAPH-NIC_NAME}
		HINTS ${LIBPACKETGRAPH-NIC_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpacketgraph-nic REQUIRED_VARS
		LIBPACKETGRAPH-NIC_LIBRARY
		LIBPACKETGRAPH-NIC_INCLUDE_DIR)
mark_as_advanced(
		LIBPACKETGRAPH-NIC_ROOT_DIR
		LIBPACKETGRAPH-NIC_LIBRARY
		LIBPACKETGRAPH-NIC_INCLUDE_DIR
)
