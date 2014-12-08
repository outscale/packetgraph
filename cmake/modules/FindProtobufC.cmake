# - Try to find protobuf-c
# Once done this will define
#  PROTOBUFC_FOUND - System has libprotobuf-c
#  PROTOBUFC_INCLUDE_DIRS - The libprotobuf-c include directories
#  PROTOBUFC_LIBRARIES - The libraries needed to use libprotobuf-c
#  PROTOBUFC_DEFINITIONS - Compiler switches required for using libprotobuf-c
#  PROTOBUFC_COMPILER - The protobuf-c compiler

find_package(PkgConfig)
pkg_check_modules(PC_PROTOBUFC_QUIET libprotobuf-c)
set(PROTOBUFC_DEFINITIONS ${PC_PROTOBUFC_CFLAGS_OTHER})

find_path(PROTOBUFC_INCLUDE_DIR google/protobuf-c/protobuf-c.h
	HINTS ${PC_PROTOBUFC_INCLUDEDIR} ${PC_PROTOBUFC_INCLUDE_DIRS}
	PATH_SUFFIXES libprotobuf-c)

find_library(PROTOBUFC_LIBRARY NAMES protobuf-c
	HINTS ${PC_PROTOBUFC_LIBDIR}
	${PC_PROTOBUFC_LIBRARY_DIRS})

find_program(PROTOCC_EXECUTABLE protoc-c)

set(PROTOBUFC_LIBRARIES ${PROTOBUFC_LIBRARY})
set(PROTOBUFC_INCLUDE_DIRS ${PROTOBUFC_INCLUDE_DIR})
set(PROTOBUFC_COMPILER ${PROTOCC_EXECUTABLE})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libprotobuf-c DEFAULT_MSG
	PROTOBUFC_LIBRARY PROTOBUFC_INCLUDE_DIR PROTOBUFC_COMPILER)
mark_as_advanced(PROTOBUFC PROTOBUFC_INCLUDE_DIR PROTOBUFC_LIBRARY)

#The following was adopted from the protobuf cmake function by
#Esben Mose Hansen <esben@ange.dk>, (C) Ange Optimization ApS 2008
function(PROTOC VAR)
	if (NOT ARGN)
		message(SEND_ERROR "error: PROTOC called without any proto files")
		return()
	endif(NOT ARGN)

	set(INCL)
	set(${VAR})
	foreach(FIL ${ARGN})
		get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
		get_filename_component(FIL_WE ${FIL} NAME_WE)
		list(APPEND ${VAR} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.c")
		list(APPEND INCL "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.h")

		add_custom_command(
			OUTPUT ${${VAR}} ${INCL}
			COMMAND  ${PROTOBUFC_COMPILER}
			ARGS --c_out  ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
			DEPENDS ${ABS_FIL}
			COMMENT "Running protocol buffer compiler on ${FIL}"
			VERBATIM
		)
		set_source_files_properties(${${VAR}} ${INCL} PROPERTIES GENERATED TRUE)
	endforeach(FIL)

	set(${VAR} ${${VAR}} PARENT_SCOPE)
endfunction(PROTOC VAR)
