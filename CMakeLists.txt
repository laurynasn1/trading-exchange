cmake_minimum_required(VERSION 3.15)
project(MatchingEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include(FetchContent)
FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main)

FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main) # need main for benchmark::benchmark

FetchContent_MakeAvailable(
        googletest
        googlebenchmark)

add_executable(matching_engine src/main.cpp)
target_include_directories(matching_engine PUBLIC src/ src/utils/)

enable_testing()
include(GoogleTest)

add_executable(unit tests/MatchingTest.cpp)
target_link_libraries(unit GTest::gtest_main)
target_include_directories(unit PUBLIC src/ src/utils/)
gtest_discover_tests(unit)

add_executable(bench tests/MatchingBenchmark.cpp)
target_link_libraries(bench benchmark::benchmark)
target_include_directories(bench PUBLIC src/ src/utils/)

add_executable(endtoend tests/EndToEndTest.cpp)
target_link_libraries(endtoend GTest::gtest_main)
target_include_directories(endtoend PUBLIC src/ src/utils/)
gtest_discover_tests(endtoend)

if ("${PERFSTAT}" STREQUAL "YES")
        target_compile_definitions(endtoend PRIVATE -DPERFSTAT)
endif()
