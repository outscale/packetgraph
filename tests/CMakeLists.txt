set(SOURCES tests.c
	test-brick-core.c
	test-brick-flow.c
	test-error.c
	test-switch.c)

add_executable(tests ${SOURCES}
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:protos>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>)
target_link_libraries (tests ${GLIB2_LIBRARIES} ${TARGET_LIBRARIES})

add_custom_target(tests_static ALL
	COMMAND ${CPPCHECK_COMMAND} ${SOURCES})
add_dependencies(tests_static tests)
