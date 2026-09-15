#include <mpi.h>
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // std::cout << "Hello world from rank " <<  world_rank << " out of " << world_size << "processes" << std::endl;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <target>" << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
    int target = atoi(argv[1]);
    if (world_rank != 0) target = 0;
    
    if (world_rank == 0)
        for (int r = 1; r < world_size; r++)
            MPI_Send(&target, 1, MPI_INT, r, 0, MPI_COMM_WORLD);
    else
        MPI_Recv(&target, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    std::cout << "(Send/Recv) Rank " << world_rank << ": " << target << std::endl;
    
    MPI_Barrier(MPI_COMM_WORLD);

    if (world_rank != 0) target = 0;

    MPI_Bcast(&target, 1, MPI_INT, 0, MPI_COMM_WORLD);
    std::cout << "(Bcast) Rank " << world_rank << ": " << target << std::endl;

    MPI_Finalize();
    return 0;
}