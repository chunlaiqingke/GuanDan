cmake_minimum_required(VERSION 3.20)
project(guandan_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- protobuf（anaconda 自带 protoc + 头文件 + 动态库）----
find_program(PROTOC_EXECUTABLE protoc HINTS /opt/anaconda3/bin /usr/local/bin /usr/bin)
find_path(PROTOBUF_INCLUDE_DIR google/protobuf/message.h HINTS /opt/anaconda3/include /usr/local/include)
find_library(PROTOBUF_LIBRARY protobuf HINTS /opt/anaconda3/lib /usr/local/lib)

if(NOT PROTOC_EXECUTABLE)
  message(FATAL_ERROR "protoc not found")
endif()
if(NOT PROTOBUF_INCLUDE_DIR OR NOT PROTOBUF_LIBRARY)
  message(FATAL_ERROR "protobuf not found (expected under /opt/anaconda3)")
endif()

# ---- spdlog（可选；缺失则退回内置极简日志，见 util/Log.h）----
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h HINTS /usr/local/include)

# ---- 运行时依赖动态库（anaconda 的 libprotobuf.dylib）----
set(CMAKE_BUILD_RPATH "/opt/anaconda3/lib")
set(CMAKE_INSTALL_RPATH "/opt/anaconda3/lib")

# ---- 生成 protobuf C++ ----
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRCS ${PROTO_GEN_DIR}/messages.pb.cc)
set(PROTO_HDRS ${PROTO_GEN_DIR}/messages.pb.h)

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${PROTO_GEN_DIR} -I ${PROTO_DIR} ${PROTO_DIR}/messages.proto
  DEPENDS ${PROTO_DIR}/messages.proto
  COMMENT "Generating protobuf C++ from messages.proto"
  VERBATIM
)

add_custom_target(gen_proto DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})

# ---- 核心库 ----
file(GLOB_RECURSE CORE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/room/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.cpp
)
list(APPEND CORE_SRCS ${PROTO_SRCS})

add_library(guandan_core STATIC ${CORE_SRCS})
add_dependencies(guandan_core gen_proto)
target_include_directories(guandan_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${PROTO_GEN_DIR}
  ${PROTOBUF_INCLUDE_DIR}
)
if(SPDLOG_INCLUDE_DIR)
  target_include_directories(guandan_core PUBLIC ${SPDLOG_INCLUDE_DIR})
endif()
target_link_libraries(guandan_core PUBLIC ${PROTOBUF_LIBRARY})

# ---- 服务端可执行 ----
add_executable(guandan_server src/main.cpp)
target_link_libraries(guandan_server PRIVATE guandan_core)

# ---- 测试 ----
enable_testing()
add_subdirectory(tests)
