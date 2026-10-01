set(MINITEST_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

add_executable(test_frame test_frame.cpp)
target_link_libraries(test_frame PRIVATE guandan_core)
target_include_directories(test_frame PRIVATE ${MINITEST_INCLUDE})
add_test(NAME test_frame COMMAND test_frame)

add_executable(test_crypto test_crypto.cpp)
target_link_libraries(test_crypto PRIVATE guandan_core)
target_include_directories(test_crypto PRIVATE ${MINITEST_INCLUDE})
add_test(NAME test_crypto COMMAND test_crypto)

add_executable(test_proto test_proto.cpp)
target_link_libraries(test_proto PRIVATE guandan_core)
target_include_directories(test_proto PRIVATE ${MINITEST_INCLUDE})
add_test(NAME test_proto COMMAND test_proto)

add_executable(test_ws_integration test_ws_integration.cpp)
target_link_libraries(test_ws_integration PRIVATE guandan_core Threads::Threads)
target_include_directories(test_ws_integration PRIVATE ${MINITEST_INCLUDE})
add_test(NAME test_ws_integration COMMAND test_ws_integration)
