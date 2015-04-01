set(SOURCES client.cc)
set (SOURCES_P)
foreach (_s ${SOURCES})
    set (SOURCES_P ${SOURCES_P} ${PROJECT_SOURCE_DIR}/api/client/${_s})
endforeach()

add_executable(api_client ${SOURCES_P})

add_dependencies(api_client api_protocol libzmq zmqpp)

set_target_properties(api_client PROPERTIES OUTPUT_NAME butterfly-client)

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_BINARY_DIR}
                    ${PROTOBUF_INCLUDE_DIR}
                    ${LIBZMQ_INCLUDE_DIR}
                    ${ZMQPP_INCLUDE_DIR})

target_link_libraries(api_client
                      api_protocol
                      pthread
                      ${PROTOBUF_LIBRARIES}
                      ${LIBZEROMQPP_LIBRARY}
                      ${GLIB2_LIBRARIES}
                      ${LIBZMQ_LIBRARIES}
                      ${ZMQPP_LIBRARIES})

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -Wall")

unset (SOURCES_P)
