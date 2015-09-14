cmake_minimum_required(VERSION 2.6)
project(packetgraph)
include(ExternalProject)

ExternalProject_Add(core
                    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core"
                    DOWNLOAD_COMMAND ""
                    INSTALL_COMMAND ""
                    CMAKE_ARGS --no-warn-unused-cli -D PG_USER_CORE_BUILD=${CMAKE_BINARY_DIR}/core-prefix/src/core-build -D PG_USER_HUB_BUILD=${CMAKE_BINARY_DIR}/hub-prefix/src/hub-build)

add_custom_target(core-package
                  COMMAND ${CMAKE_SOURCE_DIR}/tools/package.sh ${CMAKE_SOURCE_DIR}/core ${CMAKE_BINARY_DIR}/core-prefix/src/core-build core
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/core-prefix/src/core-build)

add_custom_target(tests)
add_custom_target(packages)
add_custom_target(style
                  COMMAND ${CMAKE_SOURCE_DIR}/tools/check_style.sh ${CMAKE_SOURCE_DIR})

foreach(brick hub switch nic diode print vhost vtep firewall antispoof test-graph)

ExternalProject_Add(${brick}
                    DEPENDS core
                    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${brick}"
                    DOWNLOAD_COMMAND ""
                    INSTALL_COMMAND ""
                    CMAKE_ARGS --no-warn-unused-cli -D PG_USER_CORE_BUILD=${CMAKE_BINARY_DIR}/core-prefix/src/core-build -D PG_USER_HUB_BUILD=${CMAKE_BINARY_DIR}/hub-prefix/src/hub-build)

add_custom_target(tests-${brick}
                  DEPENDS ${brick}
                  COMMAND make tests
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${brick}-prefix/src/${brick}-build)

add_custom_target(packages-${brick}
                  DEPENDS ${brick}
                  COMMAND RTE_SDK=$ENV{RTE_SDK}; ${CMAKE_SOURCE_DIR}/tools/package.sh ${CMAKE_SOURCE_DIR}/${brick} ${CMAKE_BINARY_DIR}/${brick}-prefix/src/${brick}-build ${brick}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${brick}-prefix/src/${brick}-build)

add_dependencies(tests tests-${brick})
add_dependencies(packages packages-${brick})

endforeach(brick)

