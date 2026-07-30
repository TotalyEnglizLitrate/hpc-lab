#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "kernels.hpp"

/// @file
/// Benchmark harness for Experiment 3. Times only the kernel call (buffer
/// allocation/initialisation is excluded), verifies correctness against a
/// reference sum, and prints a `RESULT ...` line that run_perf.py parses.

using Clock = std::chrono::steady_clock;

/// @brief Monotonic clock reading in seconds.
static double now_seconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

static void fill_random(std::vector<double>& v, unsigned seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto& x : v) x = dist(rng);
}

/// @brief True if a and b agree within a relative tolerance (blocked sums
/// reassociate floating-point addition, so bit-identical is not expected).
static bool close_enough(double a, double b, double rel_tol = 1e-6) {
    double diff = std::fabs(a - b);
    double scale = std::max({1.0, std::fabs(a), std::fabs(b)});
    return diff / scale < rel_tol;
}

/// @brief Part A/C: blocked sum vs. sequential-sum reference.
/// Bandwidth = 8N bytes read / time.
static int run_blocked(std::size_t N, std::size_t B) {
    if (N % B != 0) {
        std::cerr << "N must be a multiple of B\n";
        return 2;
    }
    std::vector<double> a(N);
    fill_random(a, 1);

    double t0 = now_seconds();
    double blocked = blocked_sum(a.data(), N, B);
    double t1 = now_seconds();
    double elapsed = t1 - t0;

    double seq = sequential_sum(a.data(), N);
    bool ok = close_enough(blocked, seq);

    double bandwidth_gbps = (8.0 * static_cast<double>(N)) / elapsed / 1e9;
    std::cout << "RESULT blocked N=" << N << " B=" << B
              << " time_s=" << elapsed
              << " bandwidth_GBps=" << bandwidth_gbps
              << " sum=" << blocked
              << " verified=" << (ok ? "Y" : "N") << "\n";
    return ok ? 0 : 1;
}

/// @brief Part D: AoS x-component sum. Bandwidth denominator uses the full
/// 32 bytes/particle actually streamed (not just the 8 useful bytes),
/// since that's what the memory system pays for.
static int run_aos(std::size_t N) {
    std::vector<ParticleAoS> p(N);
    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto& particle : p) {
        particle.x = dist(rng);
        particle.y = dist(rng);
        particle.z = dist(rng);
        particle.m = dist(rng);
    }

    double t0 = now_seconds();
    double s = sum_x_aos(p.data(), N);
    double t1 = now_seconds();
    double elapsed = t1 - t0;

    double bandwidth_gbps = (32.0 * static_cast<double>(N)) / elapsed / 1e9;
    std::cout << "RESULT aos N=" << N
              << " time_s=" << elapsed
              << " bandwidth_GBps=" << bandwidth_gbps
              << " sum=" << s << "\n";
    return 0;
}

/// @brief Part D: SoA x-component sum. Bandwidth = 8N bytes (unit-stride,
/// every byte loaded is useful).
static int run_soa(std::size_t N) {
    std::vector<double> x(N), y(N), z(N), m(N);
    fill_random(x, 1);
    fill_random(y, 2);
    fill_random(z, 3);
    fill_random(m, 4);

    double t0 = now_seconds();
    double s = sum_x_soa(x.data(), N);
    double t1 = now_seconds();
    double elapsed = t1 - t0;

    double bandwidth_gbps = (8.0 * static_cast<double>(N)) / elapsed / 1e9;
    std::cout << "RESULT soa N=" << N
              << " time_s=" << elapsed
              << " bandwidth_GBps=" << bandwidth_gbps
              << " sum=" << s << "\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " blocked <N> <B>\n"
                  << "  " << argv[0] << " aos <N>\n"
                  << "  " << argv[0] << " soa <N>\n";
        return 2;
    }

    std::string mode = argv[1];
    std::size_t N = std::strtoull(argv[2], nullptr, 10);

    if (mode == "blocked") {
        if (argc != 4) {
            std::cerr << "blocked needs <N> <B>\n";
            return 2;
        }
        std::size_t B = std::strtoull(argv[3], nullptr, 10);
        return run_blocked(N, B);
    } else if (mode == "aos") {
        return run_aos(N);
    } else if (mode == "soa") {
        return run_soa(N);
    }

    std::cerr << "Unknown mode: " << mode << "\n";
    return 2;
}
