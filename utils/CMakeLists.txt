set(SOURCES config.c errors.c mempool.c)

add_library(utils OBJECT ${SOURCES})
add_dependencies(utils protos)

add_custom_target(utils_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES})
add_dependencies(utils_static utils)
