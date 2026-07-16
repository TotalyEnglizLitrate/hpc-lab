#include "matrix_mult.hpp"

/// @file
/// The six loop-order variants, each expanded from LOOP3 + MATMUL_BODY.

__attribute__((noinline)) void mm_ijk(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(i, j, k, MATMUL_BODY) }
__attribute__((noinline)) void mm_ikj(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(i, k, j, MATMUL_BODY) }
__attribute__((noinline)) void mm_jik(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(j, i, k, MATMUL_BODY) }
__attribute__((noinline)) void mm_jki(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(j, k, i, MATMUL_BODY) }
__attribute__((noinline)) void mm_kij(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(k, i, j, MATMUL_BODY) }
__attribute__((noinline)) void mm_kji(Matrix &A, Matrix &B, Matrix &C, int N) { LOOP3(k, j, i, MATMUL_BODY) }
