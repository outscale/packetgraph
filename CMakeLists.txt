cmake_minimum_required(VERSION 2.6)
project(packetgraph)
include(ExternalProject)

foreach(brick core hub switch nic diode print vhost vtep firewall)

ExternalProject_Add(${brick}
                    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${brick}"
                    DOWNLOAD_COMMAND ""
                    INSTALL_COMMAND ""
                    CMAKE_ARGS --no-warn-unused-cli -D PG_USER_CORE_BUILD=${CMAKE_BINARY_DIR}/core-prefix/src/core-build -D PG_USER_HUB_BUILD=${CMAKE_BINARY_DIR}/hub-prefix/src/hub-build)

endforeach(brick)
