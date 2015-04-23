set(SOURCES bricks.c nop-brick.c collect-brick.c switch-brick.c diode-brick.c
	    vhost-user-brick.c)
set(BRICKS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/bricks)

add_library(bricks OBJECT ${SOURCES})
add_dependencies(bricks 3rdparty)
