set(SOURCES packets.c)

add_library(packets OBJECT ${SOURCES})
add_dependencies(packets 3rdparty)

add_custom_target(packets_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES})
add_dependencies(packets_static packets)
