cmake_minimum_required(VERSION 3.2)

PROTOBUF_GENERATE_CPP(echo_proto_srcs echo_proto_headers echo.proto)

add_executable(echo_server ${echo_proto_srcs} ${echo_proto_headers} server.cpp)
target_link_libraries(echo_server
    PUBLIC small_rpc
)

string(REGEX MATCH ".*/" echo_proto_dir ${echo_proto_headers})
target_include_directories(echo_server
    PUBLIC ${echo_proto_dir}
)
