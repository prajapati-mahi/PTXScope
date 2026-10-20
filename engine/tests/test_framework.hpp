#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace ptxscope::testing {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void register_test(std::string suite, std::string name, std::function<void()> func) {
        tests_.push_back({std::move(suite), std::move(name), std::move(func)});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;
        std::cout << "\n=======================================================\n";
        std::cout << "           PTXScope C++ Unit Test Suite                \n";
        std::cout << "=======================================================\n\n";

        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.suite << "." << test.name << "\n";
            bool test_ok = true;
            try {
                test.func();
            } catch (const std::exception& ex) {
                test_ok = false;
                std::cout << "  [ FAILED ]: " << ex.what() << "\n";
            } catch (...) {
                test_ok = false;
                std::cout << "  [ FAILED ]: Unknown exception caught.\n";
            }

            if (test_ok) {
                std::cout << "[       OK ] " << test.suite << "." << test.name << "\n";
                passed++;
            } else {
                std::cout << "[  FAILED  ] " << test.suite << "." << test.name << "\n";
                failed++;
            }
        }

        std::cout << "\n-------------------------------------------------------\n";
        std::cout << "Total Tests: " << tests_.size()
                  << " | Passed: " << passed
                  << " | Failed: " << failed << "\n";
        std::cout << "=======================================================\n";
        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;
};

struct TestAutoRegistrar {
    TestAutoRegistrar(std::string suite, std::string name, std::function<void()> func) {
        TestRegistry::instance().register_test(std::move(suite), std::move(name), std::move(func));
    }
};

class TestFailureException : public std::runtime_error {
public:
    explicit TestFailureException(const std::string& msg) : std::runtime_error(msg) {}
};

#define TEST_CASE(suite, name) \
    void suite##_##name##_impl(); \
    static ::ptxscope::testing::TestAutoRegistrar suite##_##name##_registrar(#suite, #name, suite##_##name##_impl); \
    void suite##_##name##_impl()

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream ss; \
            ss << __FILE__ << ":" << __LINE__ << " Assertion failed: EXPECT_TRUE(" #cond ")"; \
            throw ::ptxscope::testing::TestFailureException(ss.str()); \
        } \
    } while(0)

#define EXPECT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream ss; \
            ss << __FILE__ << ":" << __LINE__ << " Assertion failed: EXPECT_FALSE(" #cond ")"; \
            throw ::ptxscope::testing::TestFailureException(ss.str()); \
        } \
    } while(0)

#define EXPECT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::ostringstream ss; \
            ss << __FILE__ << ":" << __LINE__ << " Assertion failed: EXPECT_EQ(" #a ", " #b ") [" << (a) << " != " << (b) << "]"; \
            throw ::ptxscope::testing::TestFailureException(ss.str()); \
        } \
    } while(0)

#define EXPECT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream ss; \
            ss << __FILE__ << ":" << __LINE__ << " Assertion failed: EXPECT_NE(" #a ", " #b ")"; \
            throw ::ptxscope::testing::TestFailureException(ss.str()); \
        } \
    } while(0)

} // namespace ptxscope::testing
