set(SOURCES spsc_ring_buffer_test.cpp)

wiola_add_test(spsc_ring_buffer_test ${SOURCES})
target_link_libraries(spsc_ring_buffer_test PRIVATE wiola_core wiola_warnings)
