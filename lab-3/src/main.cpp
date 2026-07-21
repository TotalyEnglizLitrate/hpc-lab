#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <vector>
#include <random>

enum Mode {
    Branched,
    Branchless
};

void populate_vec(std::vector<int>& vec);
long long run_sum(const std::vector<int>& vec, int threshold, Mode mode);

int main(int argc, char *argv[]) {
    bool invalid_args = false;
    if (argc != 4) {
        invalid_args = true;
    }

    int vec_size, threshold;
    Mode mode;

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
    }

    if (invalid_args) {
        std::cout << "Usage: " << argv[0] << " <array size> <threshold> <branch|branchless>" << std::endl;
        return 2;
    }

    std::vector<int> vec(vec_size);
    populate_vec(vec);

    std::cout << "Sum: " << run_sum(vec, threshold, mode) << std::endl;

    return 0;
}

void populate_vec(std::vector<int>& vec) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<long long> dis(
        std::numeric_limits<long long>::min(), 
        std::numeric_limits<long long>::max()
    );

    std::generate(vec.begin(), vec.end(), [&]() {
        return dis(gen);
    });
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