cmake_minimum_required(VERSION 3.2)
project(small_rpc)

# compile flags
set(CMAKE_CXX_FLAGS -g)

# small_rpc lib
file(GLOB lib_srcs *.cpp *.h)
message(STATUS "Library " "small_rpc")
add_library(small_rpc ${lib_srcs})
target_link_libraries(small_rpc
    PUBLIC pthread
)
target_include_directories(small_rpc
    PUBLIC ${CMAKE_SOURCE_DIR}
)

# unittests
file(GLOB ut_srcs tests/test_*.cpp)
foreach(ut_src ${ut_srcs})
    string(REGEX MATCH "([^/]+).cpp" ut_name ${ut_src})
    message(STATUS "UnitTest " ${CMAKE_MATCH_1})
    add_executable(${CMAKE_MATCH_1} ${ut_src})
    target_link_libraries(${CMAKE_MATCH_1}
        PUBLIC small_rpc
    )
endforeach(ut_src)
