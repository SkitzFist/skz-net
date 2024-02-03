cmake_minimum_required(VERSION 3.14)
project(skz-net)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch ASIO 
FetchContent_Declare(asio GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git GIT_TAG asio-1-29-0)
FetchContent_MakeAvailable(asio)

add_library(${PROJECT_NAME} INTERFACE)

# Include directories for your project and ASIO
target_include_directories(
  ${PROJECT_NAME} INTERFACE 
  include/
  ${asio_SOURCE_DIR}/asio/include
)

# Define ASIO_STANDALONE to use ASIO without Boost
target_compile_definitions(${PROJECT_NAME} INTERFACE ASIO_STANDALONE)

# Linking against Windows Socket Library
target_link_libraries(${PROJECT_NAME} INTERFACE wsock32 ws2_32)
