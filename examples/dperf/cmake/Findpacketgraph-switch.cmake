# Finds packetgraph-switch includes and library
# Defines:
#  LIBPACKETGRAPH-SWITCH_INCLUDE_DIR
#  LIBPACKETGRAPH-SWITCH_LIBRARY
#  LIBPACKETGRAPH-SWITCH_FOUND

# Check if user has set a env variable to get it's own build of switch
# If variable is not found, let's get packetgraph-switch in the system.
set (PG_USER_SWITCH_BUILD $ENV{PG_SWITCH})
if ("${PG_USER_SWITCH_BUILD}" STREQUAL "")
	message ("PG_SWITCH global variable not set, searching packetgraph-switch on the system ...")

	find_path(LIBPACKETGRAPH-SWITCH_ROOT_DIR
		NAMES include/packetgraph/packetgraph.h
	)

	find_path(LIBPACKETGRAPH-SWITCH_INCLUDE_DIR
			NAMES packetgraph.h
			HINTS ${LIBPACKETGRAPH-SWITCH_ROOT_DIR}/include/packetgraph
	)

	set(LIBPACKETGRAPH-SWITCH_NAME packetgraph-switch)

	find_library(LIBPACKETGRAPH-SWITCH_LIBRARY
			NAMES ${LIBPACKETGRAPH-SWITCH_NAME}
			HINTS ${LIBPACKETGRAPH-SWITCH_ROOT_DIR}/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(libpacketgraph-switch REQUIRED_VARS
			LIBPACKETGRAPH-SWITCH_LIBRARY
			LIBPACKETGRAPH-SWITCH_INCLUDE_DIR)
	mark_as_advanced(
			LIBPACKETGRAPH-SWITCH_ROOT_DIR
			LIBPACKETGRAPH-SWITCH_LIBRARY
			LIBPACKETGRAPH-SWITCH_INCLUDE_DIR
	)
else()
	set(LIBPACKETGRAPH-SWITCH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../switch)
	set(LIBPACKETGRAPH-SWITCH_LIBRARY "${PG_USER_SWITCH_BUILD}/libpacketgraph-switch.a")
	set(LIBPACKETGRAPH-SWITCH_FOUND 1)
endif()
