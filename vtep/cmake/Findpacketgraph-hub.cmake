# Finds packetgraph-hub includes and library
# Defines:
#  LIBPACKETGRAPH-HUB_INCLUDE_DIR
#  LIBPACKETGRAPH-HUB_LIBRARY
#  LIBPACKETGRAPH-HUB_FOUND

# Check if user has set a env variable to get it's own build of hub
# If variable is not found, let's get packetgraph-hub in the system.
set (PG_USER_HUB_BUILD $ENV{PG_HUB})
if ("${PG_USER_HUB_BUILD}" STREQUAL "")
	message ("PG_HUB global variable not set, searching packetgraph-hub on the system ...")

	find_path(LIBPACKETGRAPH-HUB_ROOT_DIR
		NAMES include/packetgraph/hub.h
	)

	find_path(LIBPACKETGRAPH-HUB_INCLUDE_DIR
			NAMES hub.h
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
else()
	set(LIBPACKETGRAPH-HUB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../hub)
	set(LIBPACKETGRAPH-HUB_LIBRARY "${PG_USER_HUB_BUILD}/libpacketgraph-hub.so")
	set(LIBPACKETGRAPH-HUB_FOUND 1)
endif()
