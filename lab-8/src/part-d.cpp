#include <mpi.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size = 0;
    int world_rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int team = world_rank % 2;
    MPI_Comm team_comm;
    MPI_Comm_split(MPI_COMM_WORLD, team, world_rank, &team_comm);
    
    const int local_value = world_rank + 1;
    int reduce_total = 0;

    MPI_Reduce(
        &local_value, &reduce_total, 1, MPI_INT,
        MPI_SUM, 0, team_comm
    );

    int team_rank = 0;
    MPI_Comm_rank(team_comm, &team_rank);

    if (team_rank == 0) {
        int expected = 0;
        for (int i = 0; i < world_size; ++i) {
            if (i % 2 == team) {
                expected += (i + 1);
            }
        }
        std::cout << "Team " << team << std::endl;
        std::cout << "(MPI_Reduce) Total: " << reduce_total << std::endl;
        std::cout << "Expected total: " << expected << std::endl;

        if (reduce_total == expected) {
            std::cout << "(MPI_Reduce) PASS\n";
        } else {
            std::cout << "(MPI_Reduce) FAIL\n";
        }
    }

    MPI_Comm_free(&team_comm);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
