set(TESTS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/packetgraph/tests)
set(SOURCES tests.c
	mac.c
	test-brick-core.c
	test-brick-flow.c
	test-error.c
	test-diode.c
	test-switch.c
	test-vhost.c
	test-pkts-count.c)

add_executable(tests ${SOURCES}
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>)
target_link_libraries (tests ${GLIB2_LIBRARIES} ${TARGET_LIBRARIES})

add_custom_target(tests_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES}
	WORKING_DIRECTORY ${TESTS_SOURCE_DIR})
add_dependencies(tests_static tests)
