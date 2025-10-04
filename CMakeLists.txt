cmake_minimum_required(VERSION 3.16)
project(MyVerilogSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Core library ---
add_library(core
    src/parser.cpp
    src/lexer.cpp
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# --- Main executable ---
add_executable(MyVerilogSimMain src/main.cpp)
target_link_libraries(MyVerilogSimMain PRIVATE core)
target_include_directories(MyVerilogSimMain PRIVATE ${CMAKE_SOURCE_DIR}/include)

# --- Tests ---
add_subdirectory(tests)
