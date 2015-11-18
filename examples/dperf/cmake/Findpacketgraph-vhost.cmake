# Finds packetgraph-vhost includes and library
# Defines:
#  LIBPACKETGRAPH-VHOST_INCLUDE_DIR
#  LIBPACKETGRAPH-VHOST_LIBRARY
#  LIBPACKETGRAPH-VHOST_FOUND

# Check if user has set a env variable to get it's own build of vhost
# If variable is not found, let's get packetgraph-vhost in the system.
set (PG_USER_VHOST_BUILD $ENV{PG_VHOST})
if ("${PG_USER_VHOST_BUILD}" STREQUAL "")
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
else()
	set(LIBPACKETGRAPH-VHOST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../vhost)
	set(LIBPACKETGRAPH-VHOST_LIBRARY "${PG_USER_VHOST_BUILD}/libpacketgraph-vhost.a")
	set(LIBPACKETGRAPH-VHOST_FOUND 1)
endif()
