set(SOURCES bricks.c nop-brick.c collect-brick.c switch-brick.c diode-brick.c
	    vhost-user-brick.c hub-brick.c)

set(SOURCES_DEBUG packetsgen-brick.c)

add_library(bricks OBJECT ${SOURCES})
add_library(bricks_debug OBJECT ${SOURCES_DEBUG})
add_dependencies(bricks 3rdparty)
