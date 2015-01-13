set(SOURCES bricks.c nop-brick.c collect-brick.c switch-brick.c diode-brick.c
	    vhost-user-brick.c)

add_library(bricks OBJECT ${SOURCES})
add_dependencies(bricks 3rdparty protos)

add_custom_target(bricks_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES})
add_dependencies(bricks_static bricks)
