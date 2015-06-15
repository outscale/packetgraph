# Finds packetgraph-firewall includes and library
# Defines:
#  LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR
#  LIBPACKETGRAPH-FIREWALL_LIBRARY
#  LIBPACKETGRAPH-FIREWALL_FOUND

# Check if user has set a env variable to get it's own build of firewall
# If variable is not found, let's get packetgraph-firewall in the system.
set (PG_USER_FIREWALL_BUILD $ENV{PG_FIREWALL})
if ("${PG_USER_FIREWALL_BUILD}" STREQUAL "")
	message ("PG_FIREWALL global variable not set, searching packetgraph-firewall on the system ...")

	find_path(LIBPACKETGRAPH-FIREWALL_ROOT_DIR
		NAMES include/packetgraph/packetgraph.h
	)

	find_path(LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR
			NAMES packetgraph.h
			HINTS ${LIBPACKETGRAPH-FIREWALL_ROOT_DIR}/include/packetgraph
	)

	set(LIBPACKETGRAPH-FIREWALL_NAME packetgraph-firewall)

	find_library(LIBPACKETGRAPH-FIREWALL_LIBRARY
			NAMES ${LIBPACKETGRAPH-FIREWALL_NAME}
			HINTS ${LIBPACKETGRAPH-FIREWALL_ROOT_DIR}/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(libpacketgraph-firewall REQUIRED_VARS
			LIBPACKETGRAPH-FIREWALL_LIBRARY
			LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR)
	mark_as_advanced(
			LIBPACKETGRAPH-FIREWALL_ROOT_DIR
			LIBPACKETGRAPH-FIREWALL_LIBRARY
			LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR
	)
else()
	set(LIBPACKETGRAPH-FIREWALL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../brick-firewall)
	set(LIBPACKETGRAPH-FIREWALL_LIBRARY "${PG_USER_FIREWALL_BUILD}/libpacketgraph-firewall.so")
	set(LIBPACKETGRAPH-FIREWALL_FOUND 1)
endif()
