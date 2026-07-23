#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <vector>
#include <random>

#include <sys/prctl.h>

using Clock = std::chrono::high_resolution_clock;


enum Mode {
    Branched,
    Branchless
};

enum Dataset {
    Sorted,
    Random
};

void populate_vec(std::vector<int>& vec, Dataset dataset);
extern "C" long long run_sum(const std::vector<int>& vec, int threshold, Mode mode);

int main(int argc, char *argv[]) {
    prctl(PR_TASK_PERF_EVENTS_DISABLE);
    bool invalid_args = false;
    if (argc != 5) {
        invalid_args = true;
    }

    int vec_size, threshold;
    Mode mode;
    Dataset dataset;

    if (!invalid_args) {
        errno = 0;
        char* end = nullptr;
        long v = std::strtol(argv[1], &end, 10);

        if (errno != 0 || end == argv[1] || *end != '\0' || v <= 0 || v > std::numeric_limits<int>::max())
            invalid_args = true;
        else vec_size = static_cast<int>(v);

        errno = 0;
        v = std::strtol(argv[2], &end, 10);

        if (errno != 0 || end == argv[2] || *end != '\0' || v > std::numeric_limits<int>::max())
            invalid_args = true;
        else threshold = static_cast<int>(v);

        if (!invalid_args) {
            std::string mode_arg = std::string(argv[3]);
            if (mode_arg == "branch")
                mode = Branched;
            else if (mode_arg == "branchless")
                mode = Branchless;
            else
                invalid_args = true;
        }

        if (!invalid_args) {
            std::string dataset_arg = std::string(argv[4]);
            if (dataset_arg == "sorted")
                dataset = Sorted;
            else if (dataset_arg == "random")
                dataset = Random;
            else
                invalid_args = true;
        }
    }

    if (invalid_args) {
        std::cout << "Usage: " << argv[0] << " <array size> <threshold> <branch|branchless> <sorted|random>" << std::endl;
        return 2;
    }

    std::vector<int> vec(vec_size);
    populate_vec(vec, dataset);

    prctl(PR_TASK_PERF_EVENTS_ENABLE);
    auto start = Clock::now();
    long long _ = run_sum(vec, threshold, mode);
    auto end = Clock::now();
    prctl(PR_TASK_PERF_EVENTS_ENABLE);
    double ns = std::chrono::duration<double, std::nano>(end - start).count();
    std::cout << "Execution time: " << ns << "ns" << std::endl;

    return 0;
}

void populate_vec(std::vector<int>& vec, Dataset dataset) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<long long> dis(
        std::numeric_limits<long long>::min(), 
        std::numeric_limits<long long>::max()
    );

    std::generate(vec.begin(), vec.end(), [&]() {
        return dis(gen);
    });

    if (dataset == Sorted)
        std::sort(vec.begin(), vec.end());
}

long long run_sum(const std::vector<int>& vec, int threshold, Mode mode) {
    long long sum = 0;
    if (mode == Branched) {
        for (std::vector<int>::size_type i = 0; i < vec.size(); i++)
            if (vec[i] > threshold) sum += vec[i];
    } else {
        for (std::vector<int>::size_type i = 0; i < vec.size(); i++) {
            int m = -(vec[i] > threshold);
            sum += vec[i] & m;
        }
    }
    return sum;
}
