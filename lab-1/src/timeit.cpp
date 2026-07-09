#include "timeit.hpp"
#include <chrono>

std::chrono::nanoseconds timeit(SortFn fn, UnsignedIntVec vec, std::size_t runs) {    using namespace std::chrono;
    nanoseconds total{0};    if (runs == 0) return total;
    for (std::size_t i = 0; i < runs; ++i) {
        UnsignedIntVec tmp = vec; // sort a fresh copy each run
        auto start = high_resolution_clock::now();
        fn(tmp);
        auto end = high_resolution_clock::now();
        total += duration_cast<nanoseconds>(end - start);
    }
    return total / static_cast<long long>(runs);
}