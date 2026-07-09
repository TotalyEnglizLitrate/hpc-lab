#include "sort.hpp"
#include "timeit.hpp"

#include <iostream>
#include <random>

#define RUNS 5
#define SIZE 10

int main() {
    UnsignedIntVec vec;

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 1000);

    static std::size_t n = SIZE;
    std::cout << "Vec size: " << n << std::endl;
    vec.reserve(n);
    for (std::size_t i = 0; i < n; ++i) vec.push_back(dist(rng));

    std::vector<SortFn> fns = {bogosort, stalin_sort, sleep_sort}; 

    static std::size_t runs = RUNS;
    std::cout << "Runs: " << runs << std::endl << std::endl;
    for (SortFn fn: fns) {
        const char* name =
            (fn == stalin_sort) ? "stalin_sort" :
            (fn == bogosort) ? "bogosort" :
            "sleep_sort";
        std::cout << name << std::endl;
        auto dur = timeit(fn, vec, runs);
        std::cout << "Avg time: " << dur.count() << " ns" << std::endl << std::endl;
    }

    return 0;
}