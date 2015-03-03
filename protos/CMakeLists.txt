set(PROTOS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/protos)
set(PROTOS_OUTPUT_DIR ${PROJECT_BINARY_DIR}/protos)

# protocols targets
add_custom_target(protos-config
	DEPENDS config.pb-c.c)
add_dependencies(protos-config 3rdparty)
add_custom_target(protos-error
	DEPENDS error.pb-c.c)
add_dependencies(protos-error 3rdparty)


# protocols commands
add_custom_command(
	OUTPUT ${PROTOS_OUTPUT_DIR}/config.pb-c.c
	COMMAND ${PROTOBUFC_COMPILER} --c_out=${PROTOS_OUTPUT_DIR} config.proto
	DEPENDS config.proto
	WORKING_DIRECTORY ${PROTOS_SOURCE_DIR}
	COMMENT Generating configurations protocol buffers)

add_custom_command(
	OUTPUT ${PROTOS_OUTPUT_DIR}/error.pb-c.c
	COMMAND ${PROTOBUFC_COMPILER} --c_out=${PROTOS_OUTPUT_DIR} error.proto
	DEPENDS error.proto
	WORKING_DIRECTORY ${PROTOS_SOURCE_DIR}
	COMMENT Generating errors protocol buffers)

add_library(protos OBJECT config.pb-c.c error.pb-c.c)
