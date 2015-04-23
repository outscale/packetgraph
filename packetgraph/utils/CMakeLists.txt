set(SOURCES bitmask.c config.c errors.c mempool.c)
set(UTILS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/utils)

add_library(utils OBJECT ${SOURCES})
