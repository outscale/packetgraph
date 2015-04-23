set(SOURCES packets.c)
set(PACKETS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/packets)

add_library(packets OBJECT ${SOURCES})
add_dependencies(packets 3rdparty)
