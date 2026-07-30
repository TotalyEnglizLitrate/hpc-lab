#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <omp.h>
#include "kernels.hpp"

/// @file
/// Benchmark/driver harness for Experiment 4. `teamsize` covers Part A;
/// `sum <impl> <N> <T>` covers Parts B-D, printing a `RESULT ...` line
/// that run_perf.py parses. Timing wraps only the summation kernel, never
/// array generation or printing.

using Clock = std::chrono::steady_clock;

static double now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

static void fill_random(std::vector<double>& v, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto& x : v) x = dist(rng);
}

static bool close_enough(double a, double b, double rel_tol = 1e-9) {
    double diff = std::fabs(a - b);
    double scale = a != 0.0 ? std::fabs(a) : 1.0;
    return diff / scale < rel_tol;
}

/// @brief Part A: every thread in the team prints its id and the observed
/// team size. `clause_threads > 0` puts a num_threads() clause on the
/// pragma (Table A rows iii/v); `runtime_threads > 0` calls
/// omp_set_num_threads() first (row iv). Row i/ii need neither flag -
/// team size then comes purely from the default / OMP_NUM_THREADS env var.
static int run_teamsize(int clause_threads, int runtime_threads) {
    if (runtime_threads > 0) omp_set_num_threads(runtime_threads);

    if (clause_threads > 0) {
        #pragma omp parallel num_threads(clause_threads)
        {
            #pragma omp critical
            std::cout << "Thread " << omp_get_thread_num() << " of "
                      << omp_get_num_threads() << " (clause=" << clause_threads << ")\n";
        }
    } else {
        #pragma omp parallel
        {
            #pragma omp critical
            std::cout << "Thread " << omp_get_thread_num() << " of "
                      << omp_get_num_threads() << "\n";
        }
    }
    return 0;
}

/// @brief Parts B-D: run one named summation kernel once, timed, verified
/// against the sequential reference, and print a RESULT line.
static int run_sum(const std::string& impl, std::size_t N, int T) {
    std::vector<double> a(N);
    fill_random(a);
    double reference = sequential_sum(a.data(), N);

    double (*fn)(const double*, std::size_t, int) = nullptr;
    if (impl == "unprotected")      fn = unprotected_sum;
    else if (impl == "critical-naive") fn = critical_naive_sum;
    else if (impl == "atomic-naive")   fn = atomic_naive_sum;
    else if (impl == "critical-smart") fn = critical_smart_sum;
    else if (impl == "atomic-smart")   fn = atomic_smart_sum;
    else {
        std::cerr << "Unknown impl: " << impl << "\n";
        return 2;
    }

    double t0 = now_seconds();
    double result = fn(a.data(), N, T);
    double t1 = now_seconds();
    double elapsed = t1 - t0;

    bool ok = close_enough(result, reference);
    std::cout << "RESULT impl=" << impl << " N=" << N << " T=" << T
              << " time_s=" << elapsed
              << " sum=" << result
              << " reference=" << reference
              << " verified=" << (ok ? "Y" : "N") << "\n";
    return 0;
}

static int run_serial(std::size_t N) {
    std::vector<double> a(N);
    fill_random(a);
    double t0 = now_seconds();
    double result = sequential_sum(a.data(), N);
    double t1 = now_seconds();
    std::cout << "RESULT impl=serial N=" << N << " T=1"
              << " time_s=" << (t1 - t0)
              << " sum=" << result
              << " reference=" << result
              << " verified=Y\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " teamsize [--clause T] [--runtime T]\n"
                  << "  " << argv[0] << " serial <N>\n"
                  << "  " << argv[0] << " sum <impl> <N> <T>\n"
                  << "     impl in {unprotected, critical-naive, atomic-naive,\n"
                  << "              critical-smart, atomic-smart}\n";
        return 2;
    }

    std::string mode = argv[1];

    if (mode == "teamsize") {
        int clause = -1, runtime = -1;
        for (int i = 2; i < argc; i++) {
            if (std::strcmp(argv[i], "--clause") == 0 && i + 1 < argc) clause = std::atoi(argv[++i]);
            else if (std::strcmp(argv[i], "--runtime") == 0 && i + 1 < argc) runtime = std::atoi(argv[++i]);
        }
        return run_teamsize(clause, runtime);
    } else if (mode == "serial") {
        if (argc != 3) { std::cerr << "serial needs <N>\n"; return 2; }
        return run_serial(std::strtoull(argv[2], nullptr, 10));
    } else if (mode == "sum") {
        if (argc != 5) { std::cerr << "sum needs <impl> <N> <T>\n"; return 2; }
        std::string impl = argv[2];
        std::size_t N = std::strtoull(argv[3], nullptr, 10);
        int T = std::atoi(argv[4]);
        return run_sum(impl, N, T);
    }

    std::cerr << "Unknown mode: " << mode << "\n";
    return 2;
}
