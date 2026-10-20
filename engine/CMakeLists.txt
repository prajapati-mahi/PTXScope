cmake_minimum_required(VERSION 3.15)
project(ptxscope_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(include)

set(ENGINE_SOURCES
    src/lexer.cpp
    src/parser.cpp
    src/ir.cpp
    src/cfg.cpp
    src/analysis.cpp
    src/diagnostics.cpp
    src/optimizer.cpp
    src/passes/const_fold.cpp
    src/passes/algebraic_simp.cpp
    src/passes/dce.cpp
    src/passes/redundant_mov.cpp
    src/ptxas_integration.cpp
)

add_library(ptxscope_core STATIC ${ENGINE_SOURCES})
target_include_directories(ptxscope_core PUBLIC include)

add_executable(ptxscope src/main.cpp)
target_link_libraries(ptxscope PRIVATE ptxscope_core)

# Tests
add_subdirectory(tests)
