cmake_minimum_required(VERSION 3.13)

project(ncc)

file(GLOB HDRS "include/*.h")
file(GLOB SRC "src/*.c")

add_executable(${PROJECT_NAME} ${HDRS} ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
