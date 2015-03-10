set(SOURCES bricks.c nop-brick.c collect-brick.c switch-brick.c diode-brick.c
	    vhost-user-brick.c)
set(BRICKS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/bricks)

add_library(bricks OBJECT ${SOURCES})
add_dependencies(bricks 3rdparty)

add_custom_target(bricks_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES}
	WORKING_DIRECTORY ${BRICKS_SOURCE_DIR})
add_dependencies(bricks_static bricks)
