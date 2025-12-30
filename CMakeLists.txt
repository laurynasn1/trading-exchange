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

add_library(MatchingEngineLib INTERFACE)
target_include_directories(MatchingEngineLib INTERFACE src/ src/utils/)

add_executable(matching_engine src/main.cpp)
target_link_libraries(matching_engine PRIVATE MatchingEngineLib)

enable_testing()
include(GoogleTest)

add_executable(unit tests/MatchingTest.cpp)
target_link_libraries(unit MatchingEngineLib GTest::gtest_main)
gtest_discover_tests(unit)

add_executable(bench tests/MatchingBenchmark.cpp)
target_link_libraries(bench MatchingEngineLib benchmark::benchmark)

add_executable(endtoend tests/EndToEndTest.cpp)
target_link_libraries(endtoend MatchingEngineLib GTest::gtest_main)
gtest_discover_tests(endtoend)

if ("${PERFSTAT}" STREQUAL "YES")
        target_compile_definitions(endtoend PRIVATE -DPERFSTAT)
endif()

add_executable(client src/client/MarketDataFeedHandler.cpp)
