#pragma once

#include "sort.hpp"

#include <chrono>

std::chrono::nanoseconds timeit(SortFn fn, UnsignedIntVec vec, std::size_t runs = 1);