#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
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

void generate_vec(std::vector<int>& vec);
bool save_vec(const std::vector<int>& vec, const std::string& path);
bool load_vec(std::vector<int>& vec, const std::string& path);
void shuffle_vec(std::vector<int>& vec);
extern "C" long long run_sum(const std::vector<int>& vec, int threshold, Mode mode);

static int run_generate(int argc, char* argv[]) {
    // Usage: main generate <size> <output_file>
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " generate <size> <output_file>" << std::endl;
        return 2;
    }

    errno = 0;
    char* end = nullptr;
    long v = std::strtol(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || v <= 0 || v > std::numeric_limits<int>::max()) {
        std::cout << "Invalid size argument." << std::endl;
        return 2;
    }
    int size = static_cast<int>(v);

    std::vector<int> vec(size);
    generate_vec(vec);
    std::sort(vec.begin(), vec.end());

    if (!save_vec(vec, argv[3])) {
        std::cerr << "Failed to write to " << argv[3] << std::endl;
        return 1;
    }

    std::cout << "Wrote " << size << " sorted ints to " << argv[3] << std::endl;
    return 0;
}

static int run_bench(int argc, char* argv[]) {
    // Usage: main <input_file> <threshold> <branch|branchless> <sorted|random>
    bool invalid_args = false;
    if (argc != 5) {
        invalid_args = true;
    }

    int threshold = 0;
    Mode mode = Branched;
    Dataset dataset = Sorted;
    std::string input_file;

    if (!invalid_args) {
        input_file = argv[1];

        errno = 0;
        char* end = nullptr;
        long v = std::strtol(argv[2], &end, 10);

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
        std::cout << "Usage: " << argv[0]
                   << " <input_file> <threshold> <branch|branchless> <sorted|random>" << std::endl;
        return 2;
    }

    std::vector<int> vec;
    if (!load_vec(vec, input_file)) {
        std::cerr << "Failed to read " << input_file << std::endl;
        return 1;
    }

    // File is always sorted; shuffle in-memory (uncounted, before perf events
    // are enabled below) to produce the "random" dataset from the same data.
    if (dataset == Random)
        shuffle_vec(vec);

    prctl(PR_TASK_PERF_EVENTS_ENABLE);
    auto start = Clock::now();
    long long result = run_sum(vec, threshold, mode);
    auto end = Clock::now();
    prctl(PR_TASK_PERF_EVENTS_DISABLE);

    double ns = std::chrono::duration<double, std::nano>(end - start).count();
    std::cout << "Sum: " << result << std::endl;
    std::cout << "Execution time: " << ns << "ns" << std::endl;

    return 0;
}

int main(int argc, char *argv[]) {
    prctl(PR_TASK_PERF_EVENTS_DISABLE);

    if (argc > 1 && std::string(argv[1]) == "generate") {
        return run_generate(argc, argv);
    }

    return run_bench(argc, argv);
}

void generate_vec(std::vector<int>& vec) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<long long> dis(
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max()
    );

    std::generate(vec.begin(), vec.end(), [&]() {
        return static_cast<int>(dis(gen));
    });
}

void shuffle_vec(std::vector<int>& vec) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::shuffle(vec.begin(), vec.end(), gen);
}

bool save_vec(const std::vector<int>& vec, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    std::size_t size = vec.size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(int));

    return static_cast<bool>(out);
}

bool load_vec(std::vector<int>& vec, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    std::size_t size = 0;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (!in) return false;

    vec.resize(size);
    in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(int));

    return static_cast<bool>(in);
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
