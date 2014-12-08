# protocols targets
add_custom_target(protos-config
	DEPENDS config.pb-c.c)
add_dependencies(protos-config 3rdparty)


# protocols commands
add_custom_command(
	OUTPUT config.pb-c.c
	COMMAND ${PROTOBUFC_COMPILER} --c_out=. config.proto
	DEPENDS config.proto
	COMMENT Generating configurations protocol buffers)

add_library(protos OBJECT config.pb-c.c)
