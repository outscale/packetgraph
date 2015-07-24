# - Find NPF library
# Defines:
#  LIBNPF_INCLUDE_DIR    - where to find header files, etc.
#  LIBNPF_LIBRARY        - List of LIBNPF libraries.
#  LIBNPF_FOUND          - True if liburcu is found.

find_path(LIBNPF_ROOT_DIR
    NAMES include/npf.h
)

find_library(LIBNPF_LIBRARY
    NAMES npf
    HINTS ${LIBNPF_ROOT_DIR}/lib
)

find_path(LIBNPF_INCLUDE_DIR
    NAMES npf.h
    HINTS ${LIBNPF_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libnpf REQUIRED_VARS
	LIBNPF_LIBRARY
	LIBNPF_INCLUDE_DIR)

mark_as_advanced(
	LIBNPF_ROOT_DIR
	LIBNPF_LIBRARY
	LIBNPF_INCLUDE_DIR
)
