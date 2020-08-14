cmake_minimum_required(VERSION 3.2)
project(small_rpc)

# dependencies
find_package(GTest REQUIRED)
find_package(gflags REQUIRED)
set(Protobuf_INCLUDE_DIR ${Protobuf_DIR}/include)
set(Protobuf_LIBRARIES ${Protobuf_DIR}/lib/libprotobuf.a)
set(Protobuf_PROTOC_EXECUTABLE ${Protobuf_DIR}/bin/protoc)
find_package(Protobuf REQUIRED)

# compile flags
add_compile_options(-g)
add_compile_options(-Wall)

# small_rpc proto
file(GLOB proto_srcs protocols/*.proto)
PROTOBUF_GENERATE_CPP(proto_cpp_srcs proto_cpp_headers ${proto_srcs})

add_library(proto ${proto_cpp_srcs})
target_include_directories(proto
    PUBLIC ${Protobuf_INCLUDE_DIR}
    INTERFACE ${CMAKE_BINARY_DIR}
)
target_link_libraries(proto
    PUBLIC ${Protobuf_LIBRARIES}
)

# small_rpc lib
message(STATUS "------------- lib -------------")
file(GLOB lib_srcs base/*.cpp base/*.h protocols/*.cpp protocols/*.h net/*.cpp net/*.h)
message(STATUS "Library " "small_rpc")
add_library(small_rpc ${lib_srcs})
target_link_libraries(small_rpc
    PUBLIC pthread gflags gtest proto
)
target_include_directories(small_rpc
    PUBLIC ${CMAKE_SOURCE_DIR}
)

# unittests
message(STATUS "---------- unittests ----------")
file(GLOB ut_srcs tests/test_*.cpp)
foreach(ut_src ${ut_srcs})
    string(REGEX MATCH "([^/]+).cpp" ut_name ${ut_src})
    message(STATUS "UnitTest " ${CMAKE_MATCH_1})
    add_executable(${CMAKE_MATCH_1} ${ut_src})
    target_link_libraries(${CMAKE_MATCH_1}
        PUBLIC small_rpc
    )
endforeach(ut_src)

# examples
message(STATUS "---------- examples -----------")
file(GLOB example_dirs examples/*)
foreach(example_dir ${example_dirs})
    string(REGEX MATCH "([^/]+)$" example_name ${example_dir})
    message(STATUS "Example " ${example_name})
    add_subdirectory(examples/${example_name})
endforeach(example_dir)
message(STATUS "------------- end -------------")
