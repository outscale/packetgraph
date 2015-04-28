
set(SOURCES app.cc)

set (SOURCES_P)
foreach (_s ${SOURCES})
    set (SOURCES_P ${SOURCES_P} ${PROJECT_SOURCE_DIR}/api/server/${_s})
endforeach()

add_executable(api_server ${SOURCES_P})

set_target_properties(api_server PROPERTIES OUTPUT_NAME butterfly-server)

include_directories("${PROJECT_SOURCE_DIR}")

target_link_libraries(api_server
                      pthread
                      ${GLIB2_LIBRARIES})

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

unset (SOURCES_P)
