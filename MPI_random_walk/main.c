#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "runner.h"
#include "random_walk.h"

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Data data;
    data.l = atoi(argv[1]);
    data.a = atoi(argv[2]);
    data.b = atoi(argv[3]);
    data.n = atoi(argv[4]);
    data.N = atoi(argv[5]);
    data.pl = atof(argv[6]);
    data.pr = atof(argv[7]);
    data.pu = atof(argv[8]);
    data.pd = atof(argv[9]);
    data.prob[0] = data.pl;
    data.prob[1] = data.prob[0] + data.pr;
    data.prob[2] = data.prob[1] + data.pu;
    data.prob[3] = data.prob[2] + data.pd;
    data.rank = rank;
    data.size = size;
    // Код-запускалка с семинара (scalar/runner.h)
    // Измеряет время работы
    double time = runner_run(random_walk, &data, "walk");

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == MASTER) {
        FILE* file = fopen("stats.txt", "a+");
        fprintf(file, "%d %d %d %d %d %f %f %f %f %fs\n", data.l, data.a, data.b,
                data.n, data.N, data.pl, data.pr, data.pu, data.pd, time);
        for (int i = 0; i < data.size; i++) {
            fprintf(file, "%d: %d\n", i, data.result[i]);
        }
        fclose(file);
    }
    MPI_Finalize();
    return 0;
}
