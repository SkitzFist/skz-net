cmake_minimum_required(VERSION 3.14)
project(skz-net)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

#Fetch ASIO 
FetchContent_Declare(asio GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git GIT_TAG asio-1-29-0)
FetchContent_MakeAvailable(asio)

add_library(${PROJECT_NAME} interface)

target_include_directories(${PROJECT_NAME} INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${asio_SOURCE_DIR}/asio/include
)
