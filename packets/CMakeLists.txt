set(SOURCES packets.c)
set(PACKETS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packets)

add_library(packets OBJECT ${SOURCES})
add_dependencies(packets 3rdparty)

add_custom_target(packets_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES}
	WORKING_DIRECTORY ${PACKETS_SOURCE_DIR})
add_dependencies(packets_static packets)
