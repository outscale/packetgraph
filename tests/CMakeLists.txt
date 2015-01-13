add_executable (tests tests.c test-brick-core.c
		test-brick-flow.c
		test-error.c
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:protos>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>)
target_link_libraries (tests ${GLIB2_LIBRARIES} ${TARGET_LIBRARIES})
