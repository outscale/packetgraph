add_library(bricks OBJECT bricks.c nop-brick.c collect-brick.c switch-brick.c)
add_dependencies(bricks 3rdparty protos)
