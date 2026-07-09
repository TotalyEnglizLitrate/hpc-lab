cmake_minimum_required(VERSION 3.16)
project(lab-1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    main
    src/main.cpp
    src/timeit.cpp
    src/stalin_sort.cpp
    src/bogosort.cpp
    src/sleep_sort.cpp
)

target_include_directories(main PRIVATE include)
target_compile_options(main PRIVATE -Wall -Wextra -O2)

# sleep sort uses threads
target_link_libraries(main PRIVATE pthread)
