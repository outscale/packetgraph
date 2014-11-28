add_executable (tests test-brick-core.c  $<TARGET_OBJECTS:bricks>)
target_link_libraries (tests ${GLIB2_LIBRARIES} ${CCAN_INSTALL_DIR}/lib/libccan.a)
