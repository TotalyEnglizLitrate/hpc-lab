cmake_minimum_required(VERSION 3.16)
project(lab-5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP REQUIRED)

add_executable(main src/main.cpp src/kernels.cpp)
target_include_directories(main PRIVATE include)
target_compile_options(main PRIVATE -Wall -Wextra -O3 -march=native -ffast-math)
target_link_libraries(main PRIVATE OpenMP::OpenMP_CXX)
