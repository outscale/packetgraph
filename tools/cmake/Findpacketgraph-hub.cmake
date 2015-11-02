# Finds packetgraph-hub includes and library
# Defines:
#  LIBPACKETGRAPH-HUB_INCLUDE_DIR
#  LIBPACKETGRAPH-HUB_LIBRARY
#  LIBPACKETGRAPH-HUB_FOUND

if (NOT "${PG_HUB}" STREQUAL "")
	set(LIBPACKETGRAPH-HUB_INCLUDE_DIR ${PG_ROOT}/hub)
	set(LIBPACKETGRAPH-HUB_LIBRARY "${PG_USER_HUB_BUILD}/libpacketgraph-hub.a")
	set(LIBPACKETGRAPH-HUB_LIBRARY_SHARED "${PG_USER_HUB_BUILD}/libpacketgraph-hub.so")
	set(LIBPACKETGRAPH-HUB_FOUND 1)
    return()
endif()

if (NOT "${PG_ROOT_PATH}" STREQUAL "")
	set(LIBPACKETGRAPH-HUB_INCLUDE_DIR ${PG_ROOT}/hub)
	set(LIBPACKETGRAPH-HUB_LIBRARY "${CMAKE_BINARY_DIR}/hub/libpacketgraph-hub.a")
	set(LIBPACKETGRAPH-HUB_LIBRARY_SHARED "${CMAKE_BINARY_DIR}/libpacketgraph-hub.so")
	set(LIBPACKETGRAPH-HUB_FOUND 1)
    return()
endif()

message ("PG_HUB global variable not set, searching packetgraph-hub on the system ...")

find_path(LIBPACKETGRAPH-HUB_ROOT_DIR
	NAMES include/packetgraph/packetgraph.h
)

find_path(LIBPACKETGRAPH-HUB_INCLUDE_DIR
		NAMES packetgraph.h
		HINTS ${LIBPACKETGRAPH-HUB_ROOT_DIR}/include/packetgraph
)

set(LIBPACKETGRAPH-HUB_NAME packetgraph-hub)

find_library(LIBPACKETGRAPH-HUB_LIBRARY
		NAMES ${LIBPACKETGRAPH-HUB_NAME}
		HINTS ${LIBPACKETGRAPH-HUB_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpacketgraph-hub REQUIRED_VARS
		LIBPACKETGRAPH-HUB_LIBRARY
		LIBPACKETGRAPH-HUB_INCLUDE_DIR)
mark_as_advanced(
		LIBPACKETGRAPH-HUB_ROOT_DIR
		LIBPACKETGRAPH-HUB_LIBRARY
		LIBPACKETGRAPH-HUB_INCLUDE_DIR
)
