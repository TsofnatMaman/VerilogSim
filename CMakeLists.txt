cmake_minimum_required(VERSION 3.16)
project(VerilogSim)

# --- Core library ---
add_library(core
    src/parser.cpp
    src/lexer.cpp
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# --- Tests ---
add_subdirectory(tests)
