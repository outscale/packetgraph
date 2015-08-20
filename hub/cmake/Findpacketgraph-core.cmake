# Finds packetgraph-core includes and library
# Defines:
#  LIBPACKETGRAPH-CORE_INCLUDE_DIR
#  LIBPACKETGRAPH-CORE_LIBRARY
#  LIBPACKETGRAPH-CORE_FOUND

# Check if user has set a env variable to get it's own build of core
# If variable is not found, let's get packetgraph-core in the system.
set (PG_USER_CORE_BUILD $ENV{PG_CORE})
if ("${PG_USER_CORE_BUILD}" STREQUAL "")
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
else()
	set(LIBPACKETGRAPH-CORE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../core)
	set(LIBPACKETGRAPH-CORE_LIBRARY "${PG_USER_CORE_BUILD}/libpacketgraph-core.a")
	set(LIBPACKETGRAPH-CORE_FOUND 1)
endif()
