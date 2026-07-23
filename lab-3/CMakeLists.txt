cmake_minimum_required(VERSION 3.16)
project(lab-1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    main
    src/main.cpp
)

target_compile_options(main PRIVATE -Wall -Wextra -O0)

set_target_properties(
    main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
)

add_executable(
    bench_access_time
    src/bench_access_time.cpp
)

target_compile_options(bench_access_time PRIVATE -Wall -Wextra -O2)

set_target_properties(
    bench_access_time PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
)
