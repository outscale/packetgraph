set (protocol_descriptions message.proto
                           message_0.proto)

set(API_PROTOCOL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/api/protocol)

set (generated_sources)
foreach (_p ${protocol_descriptions})
    set (_p_path ${PROJECT_SOURCE_DIR}/api/protocol/${_p})
    string (REPLACE ".proto" ".pb.cc" generated_cc ${_p})
    string (REPLACE ".proto" ".pb.h" generated_h ${_p})
    set (generated_cc_p ${PROJECT_BINARY_DIR}/api/protocol/${generated_cc})
    set (generated_h_p ${PROJECT_BINARY_DIR}/api/protocol/${generated_h})
    add_custom_command (OUTPUT ${generated_cc_p} ${generated_h_p}
                        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${PROJECT_BINARY_DIR}/api/protocol/ --proto_path=${API_PROTOCOL_SOURCE_DIR} ${_p_path}
                        DEPENDS ${_p_path}
                        )
    set (generated_sources ${generated_sources} ${generated_cc_p} ${generated_h_p})
endforeach()

include_directories(${PROTOBUF_INCLUDE_DIR})

add_library (api_protocol STATIC ${generated_sources})

unset (protocol_descriptions)
unset (generated_sources)

