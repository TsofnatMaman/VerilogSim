add_executable(
    runTests 
    parse_module_stub_test.cpp
    port_list_tests.cpp 
    parse_expression_tests.cpp
    simulator_tests.cpp
    full_simulator_tests.cpp
)
target_include_directories(runTests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(runTests PRIVATE core)
add_test(NAME AllTests COMMAND runTests)
