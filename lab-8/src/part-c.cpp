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
    int naive_total = local_value;

    if (world_rank == 0) {
        for (int rank = 1; rank < world_size; ++rank) {
            int received_value = 0;

            MPI_Recv(
                &received_value, 1, MPI_INT, rank,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE
            );

            naive_total += received_value;
        }
    } else {
        MPI_Send(
            &local_value, 1, MPI_INT,
            0, 0, MPI_COMM_WORLD
        );
    }

    if (world_rank == 0) {
        const int expected =
            world_size * (world_size + 1) / 2;

        std::cout << "(Send/Recv) Total:   " << naive_total << '\n';
        std::cout << "Expected total:       " << expected << '\n';

        if (naive_total == expected) {
            std::cout << "(Send/Recv) PASS\n";
        } else {
            std::cout << "(Send/Recv) FAIL\n";
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    int reduce_total = 0;

    MPI_Reduce(
        &local_value, &reduce_total, 1, MPI_INT,
        MPI_SUM, 0, MPI_COMM_WORLD
    );

    if (world_rank == 0) {
        const int expected = world_size * (world_size + 1) / 2;

        std::cout << "(MPI_Reduce) Total: " << reduce_total << '\n';
        std::cout << "Expected total: " << expected << '\n';

        if (reduce_total == expected) {
            std::cout << "(MPI_Reduce) PASS\n";
        } else {
            std::cout << "(MPI_Reduce) FAIL\n";
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
