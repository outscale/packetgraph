# Finds packetgraph-vtep includes and library
# Defines:
#  LIBPACKETGRAPH-VTEP_INCLUDE_DIR
#  LIBPACKETGRAPH-VTEP_LIBRARY
#  LIBPACKETGRAPH-VTEP_FOUND

# Check if user has set a env variable to get it's own build of vtep
# If variable is not found, let's get packetgraph-vtep in the system.
set (PG_USER_VTEP_BUILD $ENV{PG_VTEP})
if ("${PG_USER_VTEP_BUILD}" STREQUAL "")
	message ("PG_VTEP global variable not set, searching packetgraph-vtep on the system ...")

	find_path(LIBPACKETGRAPH-VTEP_ROOT_DIR
		NAMES include/packetgraph/packetgraph.h
	)

	find_path(LIBPACKETGRAPH-VTEP_INCLUDE_DIR
			NAMES packetgraph.h
			HINTS ${LIBPACKETGRAPH-VTEP_ROOT_DIR}/include/packetgraph
	)

	set(LIBPACKETGRAPH-VTEP_NAME packetgraph-vtep)

	find_library(LIBPACKETGRAPH-VTEP_LIBRARY
			NAMES ${LIBPACKETGRAPH-VTEP_NAME}
			HINTS ${LIBPACKETGRAPH-VTEP_ROOT_DIR}/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(libpacketgraph-vtep REQUIRED_VARS
			LIBPACKETGRAPH-VTEP_LIBRARY
			LIBPACKETGRAPH-VTEP_INCLUDE_DIR)
	mark_as_advanced(
			LIBPACKETGRAPH-VTEP_ROOT_DIR
			LIBPACKETGRAPH-VTEP_LIBRARY
			LIBPACKETGRAPH-VTEP_INCLUDE_DIR
	)
else()
	set(LIBPACKETGRAPH-VTEP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../brick-vtep)
	set(LIBPACKETGRAPH-VTEP_LIBRARY "${PG_USER_VTEP_BUILD}/libpacketgraph-vtep.a")
	set(LIBPACKETGRAPH-VTEP_FOUND 1)
endif()
