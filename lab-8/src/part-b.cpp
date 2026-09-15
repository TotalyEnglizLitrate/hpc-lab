#include <mpi.h>

#include <cstdlib>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size = 0;
    int world_rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 2) {
        if (world_rank == 0) {
            std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int N = std::atoi(argv[1]);

    if (N <= 0 || N % world_size != 0) {
        if (world_rank == 0) {
            std::cerr << "Error: N must be positive and divisible by the "
                      << "number of MPI processes (" << world_size << ")." << std::endl;
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int local_size = N / world_size;

    std::vector<int> data;
    std::vector<int> result;

    if (world_rank == 0) {
        data.resize(N);
        result.resize(N);

        for (int i = 0; i < N; ++i) {
            data[i] = i + 1;
        }
    }

    std::vector<int> local_data(local_size);

    MPI_Scatter(
        data.data(), local_size, MPI_INT, local_data.data(),
        local_size, MPI_INT, 0, MPI_COMM_WORLD
    );

    for (int& value : local_data) {
        value *= value;
    }

    MPI_Gather(
        local_data.data(), local_size, MPI_INT, result.data(),
        local_size, MPI_INT, 0, MPI_COMM_WORLD
    );

    if (world_rank == 0) {
        bool correct = true;

        for (int i = 0; i < N; ++i) {
            const int expected = data[i] * data[i];

            if (result[i] != expected) {
                std::cout << "Mismatch at index " << i
                          << ": expected " << expected
                          << ", got " << result[i] << std::endl;
                correct = false;
            }
        }

        if (correct) {
            std::cout << "PASS: Scatter/Square/Gather verified for N = "
                      << N << " and p = " << world_size << std::endl;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}
