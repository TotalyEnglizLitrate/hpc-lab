#include "kernels.hpp"
#include <omp.h>

double sequential_sum(const double* a, std::size_t N) {
    double total = 0.0;
    for (std::size_t i = 0; i < N; i++) total += a[i];
    return total;
}

double unprotected_sum(const double* a, std::size_t N, int T) {
    double total = 0.0;
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), team = omp_get_num_threads();
        std::size_t lo = (std::size_t)tid * N / team;
        std::size_t hi = (std::size_t)(tid + 1) * N / team;
        for (std::size_t i = lo; i < hi; i++) {
            total += a[i]; // RACE: read-modify-write on shared total
        }
    }
    return total;
}

double critical_naive_sum(const double* a, std::size_t N, int T) {
    double total = 0.0;
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), team = omp_get_num_threads();
        std::size_t lo = (std::size_t)tid * N / team;
        std::size_t hi = (std::size_t)(tid + 1) * N / team;
        for (std::size_t i = lo; i < hi; i++) {
            #pragma omp critical
            total += a[i];
        }
    }
    return total;
}

double atomic_naive_sum(const double* a, std::size_t N, int T) {
    double total = 0.0;
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), team = omp_get_num_threads();
        std::size_t lo = (std::size_t)tid * N / team;
        std::size_t hi = (std::size_t)(tid + 1) * N / team;
        for (std::size_t i = lo; i < hi; i++) {
            #pragma omp atomic
            total += a[i];
        }
    }
    return total;
}

double critical_smart_sum(const double* a, std::size_t N, int T) {
    double total = 0.0;
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), team = omp_get_num_threads();
        std::size_t lo = (std::size_t)tid * N / team;
        std::size_t hi = (std::size_t)(tid + 1) * N / team;
        double partial = 0.0; // private, unprotected local loop
        for (std::size_t i = lo; i < hi; i++) partial += a[i];
        #pragma omp critical
        total += partial; // one protected combine per thread
    }
    return total;
}

double atomic_smart_sum(const double* a, std::size_t N, int T) {
    double total = 0.0;
    #pragma omp parallel num_threads(T)
    {
        int tid = omp_get_thread_num(), team = omp_get_num_threads();
        std::size_t lo = (std::size_t)tid * N / team;
        std::size_t hi = (std::size_t)(tid + 1) * N / team;
        double partial = 0.0;
        for (std::size_t i = lo; i < hi; i++) partial += a[i];
        #pragma omp atomic
        total += partial; // one protected combine per thread
    }
    return total;
}
