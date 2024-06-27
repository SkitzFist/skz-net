cmake_minimum_required(VERSION 3.14)
project(skz-net)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(
  ${PROJECT_NAME} INTERFACE 
  include/
)

target_compile_definitions(${PROJECT_NAME} INTERFACE)

# Linking against Windows Socket Library should check for OS
target_link_libraries(${PROJECT_NAME} INTERFACE ws2_32)
