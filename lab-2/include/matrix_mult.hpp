#pragma once

#include <vector>

using Matrix = std::vector<std::vector<double>>;

#define LOOP3(a, b, c, BODY) \
    for (int a = 0; a < N; a++) \
        for (int b = 0; b < N; b++) \
            for (int c = 0; c < N; c++) \
                BODY

#define MATMUL_BODY  C[i][j] += A[i][k] * B[k][j];

// noinline + the extra params (rather than globals) keep each variant a
// distinct, unambiguous symbol in the binary so `perf record`/`perf annotate`
// and `perf report --sort symbol` attribute samples to the right function
// instead of collapsing everything into one inlined blob.

__attribute__((noinline)) void mm_ijk(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_ikj(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_jik(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_jki(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_kij(Matrix &A, Matrix &B, Matrix &C, int N);
__attribute__((noinline)) void mm_kji(Matrix &A, Matrix &B, Matrix &C, int N);

struct Variant {
    const char *name;
    void (*fn)(Matrix &, Matrix &, Matrix &, int);
};

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
