#pragma once
#include <cstddef>

/// @file
/// Kernels for Experiment 3: blocked array summation (Parts A-C) and
/// AoS/SoA particle x-component summation (Part D).

/// @brief Blocked sum: N/B independent partial sums, combined in a second
/// pass. Requires N % B == 0. The two loops are kept separate (rather than
/// accumulating into `total` inline) to match the reference kernel exactly,
/// since the compiler's ability to vectorise the inner loop is what's under
/// test - fusing the combine step in would change what's being measured.
extern "C" double blocked_sum(const double* a, std::size_t N, std::size_t B);

/// @brief Plain sequential sum, used only as the correctness reference.
double sequential_sum(const double* a, std::size_t N);

/// @brief Array-of-Structures particle: all four fields interleaved
/// per-particle (x0 y0 z0 m0 x1 y1 z1 m1 ...).
struct ParticleAoS {
    double x, y, z, m;
};

/// @brief Sum of the x-component across an AoS particle array. Stride
/// between successive x's is 32 bytes (sizeof(ParticleAoS)), not 8.
double sum_x_aos(const ParticleAoS* p, std::size_t N);

/// @brief Sum of the x-component from a contiguous SoA x-array. Stride
/// between successive x's is 8 bytes (unit stride).
double sum_x_soa(const double* x, std::size_t N);
