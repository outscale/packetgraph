# Finds packetgraph-vhost includes and library
# Defines:
#  LIBPACKETGRAPH-VHOST_INCLUDE_DIR
#  LIBPACKETGRAPH-VHOST_LIBRARY
#  LIBPACKETGRAPH-VHOST_FOUND

if (NOT "${PG_VHOST}" STREQUAL "")
	set(LIBPACKETGRAPH-VHOST_INCLUDE_DIR ${PG_ROOT}/vhost)
	set(LIBPACKETGRAPH-VHOST_LIBRARY "${PG_USER_VHOST_BUILD}/libpacketgraph-vhost.a")
	set(LIBPACKETGRAPH-VHOST_LIBRARY_SHARED "${PG_USER_VHOST_BUILD}/libpacketgraph-vhost.so")
	set(LIBPACKETGRAPH-VHOST_FOUND 1)
    return()
endif()

if (NOT "${PG_ROOT_PATH}" STREQUAL "")
	set(LIBPACKETGRAPH-VHOST_INCLUDE_DIR ${PG_ROOT}/vhost)
	set(LIBPACKETGRAPH-VHOST_LIBRARY "${CMAKE_BINARY_DIR}/vhost/libpacketgraph-vhost.a")
	set(LIBPACKETGRAPH-VHOST_LIBRARY_SHARED "${CMAKE_BINARY_DIR}/libpacketgraph-vhost.so")
	set(LIBPACKETGRAPH-VHOST_FOUND 1)
    return()
endif()

message ("PG_VHOST global variable not set, searching packetgraph-vhost on the system ...")

find_path(LIBPACKETGRAPH-VHOST_ROOT_DIR
	NAMES include/packetgraph/packetgraph.h
)

find_path(LIBPACKETGRAPH-VHOST_INCLUDE_DIR
		NAMES packetgraph.h
		HINTS ${LIBPACKETGRAPH-VHOST_ROOT_DIR}/include/packetgraph
)

set(LIBPACKETGRAPH-VHOST_NAME packetgraph-vhost)

find_library(LIBPACKETGRAPH-VHOST_LIBRARY
		NAMES ${LIBPACKETGRAPH-VHOST_NAME}
		HINTS ${LIBPACKETGRAPH-VHOST_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpacketgraph-vhost REQUIRED_VARS
		LIBPACKETGRAPH-VHOST_LIBRARY
		LIBPACKETGRAPH-VHOST_INCLUDE_DIR)
mark_as_advanced(
		LIBPACKETGRAPH-VHOST_ROOT_DIR
		LIBPACKETGRAPH-VHOST_LIBRARY
		LIBPACKETGRAPH-VHOST_INCLUDE_DIR
)
