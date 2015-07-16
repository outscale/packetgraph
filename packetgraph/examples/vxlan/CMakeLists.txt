set(SOURCES vxlan.c)

add_executable(vxlan ${SOURCES}
		$<TARGET_OBJECTS:packets>
		$<TARGET_OBJECTS:utils>
		$<TARGET_OBJECTS:bricks>
		$<TARGET_OBJECTS:bricks_stubs>)

target_link_libraries (vxlan ${TARGET_LIBRARIES})
