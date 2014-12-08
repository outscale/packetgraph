# protocols targets
add_custom_target(protos-config
	DEPENDS config.pb-c.c)
add_dependencies(protos-config 3rdparty)
add_custom_target(protos-error
	DEPENDS error.pb-c.c)
add_dependencies(protos-error 3rdparty)


# protocols commands
add_custom_command(
	OUTPUT config.pb-c.c
	COMMAND ${PROTOBUFC_COMPILER} --c_out=. config.proto
	DEPENDS config.proto
	COMMENT Generating configurations protocol buffers)

add_custom_command(
	OUTPUT error.pb-c.c
	COMMAND ${PROTOBUFC_COMPILER} --c_out=. error.proto
	DEPENDS error.proto
	COMMENT Generating errors protocol buffers)

add_library(protos OBJECT config.pb-c.c error.pb-c.c)
