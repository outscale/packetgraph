# Finds packetgraph-antispoof includes and library
# Defines:
#  LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR
#  LIBPACKETGRAPH-ANTISPOOF_LIBRARY
#  LIBPACKETGRAPH-ANTISPOOF_FOUND

# Check if user has set a env variable to get it's own build of antispoof
# If variable is not found, let's get packetgraph-antispoof in the system.
if ("${PG_USER_ANTISPOOF_BUILD}" STREQUAL "")
  set (PG_USER_ANTISPOOF_BUILD $ENV{PG_ANTISPOOF})
endif()
if ("${PG_USER_ANTISPOOF_BUILD}" STREQUAL "")
	message ("PG_ANTISPOOF global variable not set, searching packetgraph-antispoof on the system ...")

	find_path(LIBPACKETGRAPH-ANTISPOOF_ROOT_DIR
		NAMES include/packetgraph/antispoof.h
	)

	find_path(LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR
			NAMES antispoof.h
			HINTS ${LIBPACKETGRAPH-ANTISPOOF_ROOT_DIR}/include/packetgraph
	)

	set(LIBPACKETGRAPH-ANTISPOOF_NAME packetgraph-antispoof)

	find_library(LIBPACKETGRAPH-ANTISPOOF_LIBRARY
			NAMES ${LIBPACKETGRAPH-ANTISPOOF_NAME}
			HINTS ${LIBPACKETGRAPH-ANTISPOOF_ROOT_DIR}/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(libpacketgraph-antispoof REQUIRED_VARS
			LIBPACKETGRAPH-ANTISPOOF_LIBRARY
			LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR)
	mark_as_advanced(
			LIBPACKETGRAPH-ANTISPOOF_ROOT_DIR
			LIBPACKETGRAPH-ANTISPOOF_LIBRARY
			LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR
	)
else()
	set(LIBPACKETGRAPH-ANTISPOOF_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../antispoof)
	set(LIBPACKETGRAPH-ANTISPOOF_LIBRARY "${PG_USER_ANTISPOOF_BUILD}/libpacketgraph-antispoof.a")
	set(LIBPACKETGRAPH-ANTISPOOF_FOUND 1)
endif()
