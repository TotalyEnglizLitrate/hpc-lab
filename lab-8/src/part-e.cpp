#include <mpi.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size = 0;
    int world_rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    const int local_value = world_rank + 1;
    int total = 0;

    MPI_Allreduce(
        &local_value, &total, 1, MPI_INT,
        MPI_SUM, MPI_COMM_WORLD
    );

    const int expected = world_size * (world_size + 1) / 2;

    std::cout 
        << "Rank " << world_rank << '\n'
        << "(MPI_Allreduce) Total: " << total << '\n'
        << "Expected total: " << expected << '\n';

    if (total == expected) {
        std::cout << "(MPI_Allreduce) PASS\n";
    } else {
        std::cout << "(MPI_Allreduce) FAIL\n";
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
