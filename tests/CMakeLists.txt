add_executable(runTests parse_module_stub_test.cpp)
target_include_directories(runTests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(runTests PRIVATE core)
