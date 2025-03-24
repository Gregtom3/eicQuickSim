cmake_minimum_required(VERSION 3.10)
project(RootExample)

find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

add_executable(main main.cpp)
target_link_libraries(main ${ROOT_LIBRARIES})
