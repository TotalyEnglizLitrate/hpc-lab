#include "kernels.hpp"
#include <vector>

extern "C" double blocked_sum(const double* a, std::size_t N, std::size_t B) {
    std::size_t nblocks = N / B;
    std::vector<double> block_sum(nblocks);

    for (std::size_t b = 0; b < nblocks; b++) {
        double s = 0.0;
        std::size_t start = b * B;
        std::size_t end = start + B;
        for (std::size_t i = start; i < end; i++)
            s += a[i];
        block_sum[b] = s;
    }

    double total = 0.0;
    for (std::size_t b = 0; b < nblocks; b++)
        total += block_sum[b];
    return total;
}

double sequential_sum(const double* a, std::size_t N) {
    double s = 0.0;
    for (std::size_t i = 0; i < N; i++)
        s += a[i];
    return s;
}

double sum_x_aos(const ParticleAoS* p, std::size_t N) {
    double s = 0.0;
    for (std::size_t i = 0; i < N; i++)
        s += p[i].x;
    return s;
}

double sum_x_soa(const double* x, std::size_t N) {
    double s = 0.0;
    for (std::size_t i = 0; i < N; i++)
        s += x[i];
    return s;
}
