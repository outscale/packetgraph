add_library(utils OBJECT config.c errors.c)
add_dependencies(utils protos)
