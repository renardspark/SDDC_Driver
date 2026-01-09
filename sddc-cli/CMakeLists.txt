cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_STANDARD 20)

include_directories("." "../Core")

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

add_executable(sddc-cli sddc-cli.cpp)
target_link_libraries(sddc-cli PRIVATE SDDC_CORE argparse)