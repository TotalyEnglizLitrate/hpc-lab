cmake_minimum_required(VERSION 3.10)
project(lab2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -O2 so the loop-order differences are actually representative of real
# usage (unoptimized code is dominated by other overhead).
# -g keeps debug symbols so `perf record`/`perf annotate`/`perf report` can
# map samples back to source lines instead of raw addresses.
# -fno-omit-frame-pointer keeps call-graph unwinding (`perf record -g`)
# reliable without needing DWARF-based unwinding.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -fno-omit-frame-pointer")

add_executable(
        main
        src/main.cpp
        src/matrix_mult.cpp
)
target_include_directories(main PRIVATE include)
