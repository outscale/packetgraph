add_library(bricks OBJECT bricks.c nop-brick.c)
add_dependencies(bricks 3rdparty)
