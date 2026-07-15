#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include "matrix_mult.hpp"

static Matrix make_random(int N, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    Matrix M(N, std::vector<int>(N));
    for (auto &row : M)
        for (auto &v : row)
            v = dist(rng);
    return M;
}

static void run_one(const Variant &v, int N) {
    Matrix A = make_random(N, 1);
    Matrix B = make_random(N, 2);
    Matrix C(N, std::vector<int>(N, 0));

    for (auto &row : C) std::fill(row.begin(), row.end(), 0.0);
    v.fn(A, B, C, N);

    double checksum = 0.0;
    for (auto &row : C) for (double x : row) checksum += x;
}

int main(int argc, char **argv) {
    int N = argc > 2 ? std::atoi(argv[2]) : 512;

    if (argc > 1 && std::string(argv[1]) != "all") {
        // Single-variant mode: `./main ikj 512 20`
        // Run this under perf so the whole profile is one clean symbol,
        // e.g.:  perf stat -e cache-references,cache-misses,instructions,cycles ./main ikj
        //        perf record -g -- ./main ikj && perf report
        std::string name = argv[1];
        for (auto &v : variants()) {
            if (name == v.name) {
                run_one(v, N);
                return 0;
            }
        }
        std::cerr << "unknown variant: " << name << "\n";
        return 1;
    }

    // "all" mode: quick side-by-side wall-clock comparison, not meant for
    // perf record (samples from all six would land in one profile).
    for (auto &v : variants())
        run_one(v, N);
    return 0;
}
