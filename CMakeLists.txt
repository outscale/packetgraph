cmake_minimum_required(VERSION 2.6)
project(packetgraph)

set(PG_ROOT_PATH ${CMAKE_SOURCE_DIR})

set(bricks
    core
    antispoof
    diode
    firewall
    integration # not a real brick but same behavior
    nic
    print
    switch
    vtep
    vhost)
add_custom_target(tests)
add_custom_target(bench)

foreach(brick IN LISTS bricks)
    # Include brick building
    add_subdirectory(${brick})
    # Include tests for all bricks
    add_dependencies(tests packetgraph-${brick}-tests-run)
    # Directly build a specific brick
    add_custom_target(${brick})
    add_dependencies(${brick} packetgraph-${brick})
    # Directly run tests of a specific brick
    add_custom_target(tests-${brick})
    add_dependencies(tests-${brick} packetgraph-${brick}-tests-run)
    # Include benchmarks for all bricks
    add_dependencies(bench packetgraph-${brick}-bench-run)
endforeach(brick)

add_custom_target(style
        COMMAND ${PG_ROOT_PATH}/tools/check_style.sh ${PG_ROOT_PATH}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

# Check a lot of things, useful for pull request
add_custom_target(check-all)
add_dependencies(check-all all tests style bench)
