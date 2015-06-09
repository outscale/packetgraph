set(TESTS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/tests)

add_definitions( -DPROJECT_ROOT="${PROJECT_SOURCE_DIR}" )

set(SOURCES test-nic-stub.c)

add_library(bricks_stubs_test OBJECT ${SOURCES})

set(SOURCES tests.c
	mac.c
	test-brick-core.c
	test-brick-flow.c
	test-error.c
	test-diode.c
	test-switch.c
	test-vhost.c
	test-pkts-count.c
	test-hub.c
	test-nic.c
	test-firewall.c)

add_executable(tests ${SOURCES}
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>
		$<TARGET_OBJECTS:bricks_stubs_test>)
target_link_libraries (tests ${TARGET_LIBRARIES})
