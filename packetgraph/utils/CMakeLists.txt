set(SOURCES bitmask.c config.c errors.c mempool.c)
set(UTILS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/utils)

add_library(utils OBJECT ${SOURCES})

add_custom_target(utils_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES}
      	WORKING_DIRECTORY ${UTILS_SOURCE_DIR})

add_dependencies(utils_static utils)
