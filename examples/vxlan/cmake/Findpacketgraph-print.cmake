# Finds packetgraph-print includes and library
# Defines:
#  LIBPACKETGRAPH-PRINT_INCLUDE_DIR
#  LIBPACKETGRAPH-PRINT_LIBRARY
#  LIBPACKETGRAPH-PRINT_FOUND

# Check if user has set a env variable to get it's own build of print
# If variable is not found, let's get packetgraph-print in the system.
set (PG_USER_PRINT_BUILD $ENV{PG_PRINT})
if ("${PG_USER_PRINT_BUILD}" STREQUAL "")
	message ("PG_PRINT global variable not set, searching packetgraph-print on the system ...")

	find_path(LIBPACKETGRAPH-PRINT_ROOT_DIR
		NAMES include/packetgraph/packetgraph.h
	)

	find_path(LIBPACKETGRAPH-PRINT_INCLUDE_DIR
			NAMES packetgraph.h
			HINTS ${LIBPACKETGRAPH-PRINT_ROOT_DIR}/include/packetgraph
	)

	set(LIBPACKETGRAPH-PRINT_NAME packetgraph-print)

	find_library(LIBPACKETGRAPH-PRINT_LIBRARY
			NAMES ${LIBPACKETGRAPH-PRINT_NAME}
			HINTS ${LIBPACKETGRAPH-PRINT_ROOT_DIR}/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(libpacketgraph-print REQUIRED_VARS
			LIBPACKETGRAPH-PRINT_LIBRARY
			LIBPACKETGRAPH-PRINT_INCLUDE_DIR)
	mark_as_advanced(
			LIBPACKETGRAPH-PRINT_ROOT_DIR
			LIBPACKETGRAPH-PRINT_LIBRARY
			LIBPACKETGRAPH-PRINT_INCLUDE_DIR
	)
else()
	set(LIBPACKETGRAPH-PRINT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../brick-print)
	set(LIBPACKETGRAPH-PRINT_LIBRARY "${PG_USER_PRINT_BUILD}/libpacketgraph-print.a")
	set(LIBPACKETGRAPH-PRINT_FOUND 1)
endif()
