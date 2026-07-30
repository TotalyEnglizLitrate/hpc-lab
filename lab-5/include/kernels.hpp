#pragma once
#include <cstddef>

/// @file
/// Kernels for Experiment 4: OpenMP fork-join basics and synchronising a
/// parallel array sum (critical vs. atomic, naive vs. smart granularity).

/// @brief Plain sequential sum, the trusted reference used everywhere.
double sequential_sum(const double* a, std::size_t N);

/// @brief Part B: parallel sum split by thread id into contiguous chunks,
/// accumulated directly into one shared total with no protection at all.
/// Deliberately racy - do not "fix" this one.
double unprotected_sum(const double* a, std::size_t N, int T);

/// @brief Part C: critical wraps the total += a[i] update on every single
/// iteration (protected section executes N times).
double critical_naive_sum(const double* a, std::size_t N, int T);

/// @brief Part C: same as critical_naive_sum but with atomic instead of
/// critical (protected section still executes N times, but atomic uses a
/// hardware CAS/fetch-add instead of acquiring a lock).
double atomic_naive_sum(const double* a, std::size_t N, int T);

/// @brief Part C: each thread accumulates its chunk into a private partial
/// sum with an ordinary unprotected loop, then does one critical-protected
/// combine (`total += partial`) at the end - protected section executes
/// only T times total, not N times.
double critical_smart_sum(const double* a, std::size_t N, int T);

/// @brief Part C/D: same private-partial-then-combine idea as
/// critical_smart_sum, but the one combine per thread uses atomic instead
/// of critical.
double atomic_smart_sum(const double* a, std::size_t N, int T);
