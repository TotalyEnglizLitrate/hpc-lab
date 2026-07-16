#pragma once

#include <vector>

/// Dense row-major N x N matrix.
using Matrix = std::vector<std::vector<double>>;

/// Triple loop nest a->b->c (outer->inner) over [0,N), running BODY innermost.
#define LOOP3(a, b, c, BODY) \
    for (int a = 0; a < N; a++) \
        for (int b = 0; b < N; b++) \
            for (int c = 0; c < N; c++) \
                BODY

/// Inner accumulation, identical across variants; only the loop order changes.
#define MATMUL_BODY  C[i][j] += A[i][k] * B[k][j];

/// @name Loop-order variants of C = A*B (N x N, row-major)
/// Each name gives the loop nesting outer->inner. `noinline` + explicit params
/// keep every variant a distinct symbol so perf attributes samples correctly
/// instead of collapsing them into one inlined blob.
/// @{
__attribute__((noinline)) void mm_ijk(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_ikj(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_jik(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_jki(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_kij(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_kji(Matrix &A, Matrix &B, Matrix &C, int N);
/// @}

/// @brief A named loop-order kernel: its label and function pointer.
struct Variant {
    const char *name;
    void (*fn)(Matrix &, Matrix &, Matrix &, int);
};

/// @brief All six loop-order variants, in ijk..kji order.
inline const std::vector<Variant> &variants() {
    static const std::vector<Variant> v = {
        {"ijk", mm_ijk},
        {"ikj", mm_ikj},
        {"jik", mm_jik},
        {"jki", mm_jki},
        {"kij", mm_kij},
        {"kji", mm_kji},
    };
    return v;
}
