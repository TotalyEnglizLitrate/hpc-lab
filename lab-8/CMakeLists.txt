cmake_minimum_required(VERSION 3.16)
project(lab-5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED)

foreach(part IN ITEMS a b c d e)
    add_executable(part-${part} src/part-${part}.cpp)

    target_compile_options(
        part-${part} PRIVATE
        -Wall -Wextra -O3 -march=native
    )

    target_link_libraries(part-${part} PRIVATE MPI::MPI_CXX)
endforeach()
