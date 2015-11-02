# Finds packetgraph-core includes and library
# Defines:
#  LIBPACKETGRAPH-CORE_INCLUDE_DIR
#  LIBPACKETGRAPH-CORE_LIBRARY
#  LIBPACKETGRAPH-CORE_FOUND

if (NOT "${PG_CORE}" STREQUAL "")
	set(LIBPACKETGRAPH-CORE_INCLUDE_DIR ${PG_ROOT}/core)
	set(LIBPACKETGRAPH-CORE_LIBRARY "${PG_USER_CORE_BUILD}/libpacketgraph-core.a")
	set(LIBPACKETGRAPH-CORE_LIBRARY_SHARED "${PG_USER_CORE_BUILD}/libpacketgraph-core.so")
	set(LIBPACKETGRAPH-CORE_FOUND 1)
    return()
endif()

if (NOT "${PG_ROOT_PATH}" STREQUAL "")
	set(LIBPACKETGRAPH-CORE_INCLUDE_DIR ${PG_ROOT}/core)
	set(LIBPACKETGRAPH-CORE_LIBRARY "${CMAKE_BINARY_DIR}/core/libpacketgraph-core.a")
	set(LIBPACKETGRAPH-CORE_LIBRARY_SHARED "${CMAKE_BINARY_DIR}/libpacketgraph-core.so")
	set(LIBPACKETGRAPH-CORE_FOUND 1)
    return()
endif()

message ("PG_CORE global variable not set, searching packetgraph-core on the system ...")

find_path(LIBPACKETGRAPH-CORE_ROOT_DIR
	NAMES include/packetgraph/packetgraph.h
)

find_path(LIBPACKETGRAPH-CORE_INCLUDE_DIR
		NAMES packetgraph.h
		HINTS ${LIBPACKETGRAPH-CORE_ROOT_DIR}/include/packetgraph
)

set(LIBPACKETGRAPH-CORE_NAME packetgraph-core)

find_library(LIBPACKETGRAPH-CORE_LIBRARY
		NAMES ${LIBPACKETGRAPH-CORE_NAME}
		HINTS ${LIBPACKETGRAPH-CORE_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpacketgraph-core REQUIRED_VARS
		LIBPACKETGRAPH-CORE_LIBRARY
		LIBPACKETGRAPH-CORE_INCLUDE_DIR)
mark_as_advanced(
		LIBPACKETGRAPH-CORE_ROOT_DIR
		LIBPACKETGRAPH-CORE_LIBRARY
		LIBPACKETGRAPH-CORE_INCLUDE_DIR
)
