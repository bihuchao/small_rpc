cmake_minimum_required(VERSION 3.2)

PROTOBUF_GENERATE_CPP(echo_proto_srcs echo_proto_headers echo.proto)

file(GLOB example_srcs *.cpp)

message(STATUS "-------- echo examples --------")
foreach(example_src ${example_srcs})
    string(REGEX MATCH "([^/]+).cpp" example_name ${example_src})
    message(STATUS "EchoExample " ${CMAKE_MATCH_1})
    set(example_target ${CMAKE_MATCH_1})
    add_executable(${example_target} ${echo_proto_srcs} ${echo_proto_headers} ${example_src})
    target_link_libraries(${example_target}
        PUBLIC small_rpc
    )
    string(REGEX MATCH ".*/" echo_proto_dir ${echo_proto_headers})
    target_include_directories(${example_target}
        PUBLIC ${echo_proto_dir}
    )
endforeach(example_src)
