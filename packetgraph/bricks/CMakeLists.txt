set(SOURCES bricks.c nop-brick.c collect-brick.c switch-brick.c diode-brick.c
	    vhost-user-brick.c hub-brick.c packetsgen-brick.c nic-brick.c
	    firewall-brick.c vtep-brick.c)

add_library(bricks OBJECT ${SOURCES})
add_dependencies(bricks 3rdparty)

set(SOURCES nic-brick-wrap.c)

add_library(bricks_stubs OBJECT ${SOURCES})
add_dependencies(bricks_stubs 3rdparty)

