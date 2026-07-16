#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "matrix_mult.hpp"

/// @file
/// Benchmark harness: times each loop-order kernel, verifies correctness
/// against ijk, and runs a cache-line stride probe.

/// @brief N x N matrix filled with uniform doubles in [-100, 100).
/// @param seed RNG seed (use distinct seeds for independent operands).
static Matrix make_random(int N, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-100., 100.);
    Matrix M(N, std::vector<double>(N));
    for (auto &row : M)
        for (auto &v : row)
            v = dist(rng);
    return M;
}

/// @brief True if every element of X and Y agrees within @p rel_tol (relative).
static bool matrices_close(const Matrix &X, const Matrix &Y, double rel_tol = 1e-9) {
    int N = static_cast<int>(X.size());
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            double a = X[i][j], b = Y[i][j];
            double diff = std::fabs(a - b);
            double scale = std::max({1.0, std::fabs(a), std::fabs(b)});
            if (diff / scale > rel_tol) return false;
        }
    return true;
}

/// @brief Monotonic clock reading in seconds.
static double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

/// @brief Time one variant's kernel (alloc/init excluded) and print
/// `RESULT <name> <N> <time_s> <gflops>`.
static void run_timed(const Variant &v, int N) {
    Matrix A = make_random(N, 1);
    Matrix B = make_random(N, 2);
    Matrix C(N, std::vector<double>(N, 0.0));

    double t0 = now_seconds();
    v.fn(A, B, C, N);
    double t1 = now_seconds();

    double elapsed = t1 - t0;
    double gflops = (2.0 * N * N * N) / elapsed / 1e9;
    std::cout << "RESULT " << v.name << " " << N << " " << elapsed << " " << gflops << "\n";
}

/// @brief Check every variant against the ijk reference; print one VERIFY
/// line each and exit non-zero on any mismatch.
static void run_verify(int N) {
    Matrix A = make_random(N, 1);
    Matrix B = make_random(N, 2);

    Matrix Cref(N, std::vector<double>(N, 0.0));
    mm_ijk(A, B, Cref, N);

    bool all_ok = true;
    for (auto &v : variants()) {
        Matrix C(N, std::vector<double>(N, 0.0));
        v.fn(A, B, C, N);
        bool ok = matrices_close(C, Cref);
        all_ok = all_ok && ok;
        std::cout << "VERIFY " << v.name << " " << (ok ? "PASS" : "FAIL") << "\n";
    }
    if (!all_ok) {
        std::cerr << "one or more variants failed correctness check\n";
        std::exit(1);
    }
}

/// @brief Cache-line probe: touch every `stride`-th double in a large buffer,
/// @p rounds times, printing `STRIDE <stride> <bytes> <ns_per_touch>` per
/// stride. Per-touch cost plateaus once the stride exceeds the cache line
/// (each touch becomes its own miss instead of riding a neighbour's fetch).
static void run_stride(long total_doubles, const std::vector<long> &strides, int rounds) {
    std::vector<double> buf(total_doubles);
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto &x : buf) x = dist(rng);

    for (long stride : strides) {
        long touches_per_round = total_doubles / stride;
        if (touches_per_round < 1) continue;

        volatile double sink = 0.0;
        double t0 = now_seconds();
        for (int r = 0; r < rounds; r++) {
            for (long i = 0; i < touches_per_round; i++) {
                sink += buf[i * stride];
            }
        }
        double t1 = now_seconds();
        (void)sink;

        long total_touches = touches_per_round * rounds;
        double ns_per_touch = (t1 - t0) * 1e9 / static_cast<double>(total_touches);
        std::cout << "STRIDE " << stride << " " << (stride * sizeof(double))
                   << " " << ns_per_touch << "\n";
    }
}

/// @brief CLI: `verify [N]` | `stride [doubles] [rounds] [strides...]` |
/// `<variant> [N]` | `all [N]`.
int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "verify") {
        int N = argc > 2 ? std::atoi(argv[2]) : 256;
        run_verify(N);
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "stride") {
        // stride [doubles] [rounds] [strides...]; default 2^24 doubles (128 MB, > L3)
        long total_doubles = argc > 2 ? std::atol(argv[2]) : (1L << 24);
        int rounds = argc > 3 ? std::atoi(argv[3]) : 5;
        std::vector<long> strides;
        if (argc > 4) {
            for (int i = 4; i < argc; i++) strides.push_back(std::atol(argv[i]));
        } else {
            strides = {1, 2, 4, 8, 16, 32, 64, 128, 256};
        }
        run_stride(total_doubles, strides, rounds);
        return 0;
    }

    int N = argc > 2 ? std::atoi(argv[2]) : 512;

    if (argc > 1 && std::string(argv[1]) != "all") {
        // Single-variant mode: `./main ikj 2048`.
        std::string name = argv[1];
        for (auto &v : variants()) {
            if (name == v.name) {
                run_timed(v, N);
                return 0;
            }
        }
        std::cerr << "unknown variant: " << name << "\n";
        return 1;
    }

    // "all" mode: one RESULT line per variant.
    for (auto &v : variants())
        run_timed(v, N);
    return 0;
}
