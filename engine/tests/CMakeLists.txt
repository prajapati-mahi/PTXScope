set(TEST_SOURCES
    test_main.cpp
    test_lexer.cpp
    test_parser.cpp
    test_ir.cpp
    test_cfg.cpp
    test_analysis.cpp
    test_optimizer.cpp
    test_correctness.cpp
)

add_executable(ptxscope_tests ${TEST_SOURCES})
target_include_directories(ptxscope_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../include)
target_link_libraries(ptxscope_tests PRIVATE ptxscope_core)

enable_testing()
add_test(NAME AllTests COMMAND ptxscope_tests)
