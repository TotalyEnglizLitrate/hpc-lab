#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <numeric>
#include <algorithm>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

double benchRollingAvg(const std::vector<int>& v, const std::vector<size_t>& indices, int passes) {
    volatile long long sum = 0; // prevents the compiler from optimizing the loop away
    double avgNsPerAccess = 0.0;
    long long count = 0;

    for (int p = 0; p < passes; ++p) {
        auto start = Clock::now();

        for (size_t i = 0; i < indices.size(); ++i) {
            sum += v[indices[i]];
        }

        auto end = Clock::now();
        double ns = std::chrono::duration<double, std::nano>(end - start).count();
        double nsPerAccess = ns / (double)indices.size();

        ++count;
        avgNsPerAccess += (nsPerAccess - avgNsPerAccess) / (double)count;
    }

    (void)sum;
    return avgNsPerAccess;
}

int main(int argc, char** argv) {
    size_t size = 10000000;
    if (argc > 1) {
        size = std::strtoull(argv[1], nullptr, 10);
        if (size == 0) {
            std::cerr << "Invalid size argument, using default.\n";
            size = 10000000;
        }
    }

    // Number of passes to average over
    int passes = 30;
    if (argc > 2) {
        int p = std::atoi(argv[2]);
        if (p > 0) passes = p;
    }

    std::vector<int> v(size);
    std::iota(v.begin(), v.end(), 0);

    // Sequential index order and a pre-shuffled index order (shuffle cost
    // isn't timed since it happens before the benchmark loop)
    std::vector<size_t> seqIndices(size);
    std::iota(seqIndices.begin(), seqIndices.end(), 0);

    std::vector<size_t> randIndices = seqIndices;
    std::mt19937 rng(42);
    std::shuffle(randIndices.begin(), randIndices.end(), rng);

    double seqNsPerAccess = benchRollingAvg(v, seqIndices, passes);
    double randNsPerAccess = benchRollingAvg(v, randIndices, passes);

    double bytes = (double)size * sizeof(int);
    std::cout << size << "," << bytes << ","
              << seqNsPerAccess << "," << randNsPerAccess << ","
              << (randNsPerAccess / seqNsPerAccess) << "\n";

    return 0;
}
