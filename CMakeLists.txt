cmake_minimum_required(VERSION 3.15)
project(MatchingEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
set(CMAKE_BUILD_TYPE Release)

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

add_library(MatchingEngineLib STATIC src/OrderBook.cpp src/MatchingEngine.cpp)
target_include_directories(MatchingEngineLib PUBLIC src/ src/utils)

add_executable(matching_engine src/main.cpp)
target_link_libraries(matching_engine MatchingEngineLib)

enable_testing()

add_executable(tests tests/MatchingTest.cpp tests/EndToEndTest.cpp)
target_link_libraries(tests MatchingEngineLib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)

add_executable(bench tests/MatchingBenchmark.cpp)
target_link_libraries(bench MatchingEngineLib benchmark::benchmark)
