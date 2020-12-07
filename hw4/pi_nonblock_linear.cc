#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

long long calculate(long long local_toss)
{
    long long local_sum = 0;
    unsigned int seed = time(NULL);

    for (long long i = 0; i < local_toss; i++)
    {
        float x = (float)rand_r(&seed) / RAND_MAX;
        float y = (float)rand_r(&seed) / RAND_MAX;
        if (x * x + y * y <= 1)
            local_sum++;
    }
    return local_sum;
}

int main(int argc, char **argv)
{
    // --- DON'T TOUCH ---
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();
    double pi_result;
    long long int tosses = atoi(argv[1]);
    int world_rank, world_size;
    // ---

    // TODO: MPI init
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    long long local_sum = 0;

    if (world_rank > 0)
    {
        // TODO: MPI workers
        long long local_toss = tosses / world_size;
        local_sum = calculate(local_toss);
        // send msg
        MPI_Send(&local_sum, 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }
    else if (world_rank == 0)
    {
        // TODO: non-blocking MPI communication.
        // Use MPI_Irecv, MPI_Wait or MPI_Waitall.
        long long local_toss = tosses - (world_size - 1) * (tosses / world_size);
        local_sum = calculate(local_toss);

        long long *tmp_arr = (long long *)malloc(sizeof(long long int) * world_size);
        MPI_Request requests[world_size - 1];
        MPI_Status status[world_size - 1];
        for (int i = 1; i < world_size; i++)
        {
            MPI_Irecv(&tmp_arr[i], 1, MPI_LONG_LONG, i, 0, MPI_COMM_WORLD, requests);
        }
        MPI_Waitall(world_size - 1, requests, status);
        for (int i = 1; i < world_size; i++)
        {
            local_sum += tmp_arr[i];
        }
    }

    if (world_rank == 0)
    {
        // TODO: PI result
        pi_result = 4 * local_sum / (double)tosses;
        // --- DON'T TOUCH ---
        double end_time = MPI_Wtime();
        printf("%lf\n", pi_result);
        printf("MPI running time: %lf Seconds\n", end_time - start_time);
        // ---
    }

    MPI_Finalize();
    return 0;
}
