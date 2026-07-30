cmake_minimum_required(VERSION 3.16)
project(lab-4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# One executable per compiler-flag combination, all built from the
# identical source (kernels.cpp/main.cpp) - only the flags differ, so any
# timing/vectorisation difference between targets is attributable to the
# flags alone. -fopt-info-vec / -fopt-info-vec-missed is emitted on stderr
# at build time; run_perf.py captures it per target as the vectorisation
# report.
function(add_variant name)
    add_executable(${name} src/main.cpp src/kernels.cpp)
    target_include_directories(${name} PRIVATE include)
    target_compile_options(${name} PRIVATE -Wall -Wextra ${ARGN}
        -fopt-info-vec)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
endfunction()

# Table A: optimisation-level sweep.
add_variant(main_O0             -O0)
add_variant(main_O1             -O1)
add_variant(main_O2             -O2)
add_variant(main_O3             -O3)
add_variant(main_O3_native      -O3 -march=native)
add_variant(main_O3_native_fast -O3 -march=native -ffast-math)

# Table B: scalar vs. vector build for the perf/objdump comparison.
# main_O3_native above doubles as the "vector build".
add_variant(main_scalar         -O2 -fno-tree-vectorize)

# Tables C & D: fixed "best flags" build (as picked from table A)
add_variant(main_bestflags      -O3 -march=native -ffast-math)
