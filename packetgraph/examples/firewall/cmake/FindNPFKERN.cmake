# - Find NPFKERN library
# Defines:
#  LIBNPFKERN_INCLUDE_DIR    - where to find header files, etc.
#  LIBNPFKERN_LIBRARY        - List of LIBNPFKERN libraries.
#  LIBNPFKERN_FOUND          - True if liburcu is found.

find_path(LIBNPFKERN_ROOT_DIR
    NAMES include/npf.h
)

find_library(LIBNPFKERN_LIBRARY
    NAMES npfkern
    HINTS ${LIBNPFKERN_ROOT_DIR}/lib
)

find_path(LIBNPFKERN_INCLUDE_DIR
    NAMES npf.h
    HINTS ${LIBNPFKERN_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libnpf REQUIRED_VARS
	LIBNPFKERN_LIBRARY
	LIBNPFKERN_INCLUDE_DIR)

mark_as_advanced(
	LIBNPFKERN_ROOT_DIR
	LIBNPFKERN_LIBRARY
	LIBNPFKERN_INCLUDE_DIR
)
