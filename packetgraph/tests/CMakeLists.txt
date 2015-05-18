set(TESTS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/tests)

add_definitions( -DPROJECT_ROOT="${PROJECT_SOURCE_DIR}" )

set(SOURCES tests.c
	mac.c
	test-brick-core.c
	test-brick-flow.c
	test-error.c
	test-diode.c
	test-switch.c
	test-vhost.c
	test-pkts-count.c
	test-hub.c)

add_executable(tests ${SOURCES}
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>)
target_link_libraries (tests ${TARGET_LIBRARIES})
