cmake_minimum_required (VERSION 3.10.2)
project (yjson_test)

# add_definitions(-std=c++17)
set(CMAKE_CXX_STANDARD 17) 

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_library(yjson yjson.cpp)

set(TEST_LIST test.cpp)
add_executable(yjson_test ${TEST_LIST})

target_link_libraries(yjson_test yjson -lpthread)