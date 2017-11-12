//
//  main.c
//  MPI_IO
//
//  Created by Dmitry Belyaev on 12.11.17.
//  Copyright © 2017 Dmitry Belyaev. All rights reserved.
//

#include <stdio.h>
#include <mpi.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>

#define MASTER 0

typedef struct ProcessData {
    int rank;
    int size;
    int l;
    int a;
    int b;
    int N;
} ProcessData;

typedef struct Particle {
    int r;
    int x;
    int y;
} Particle;

void generate_points(ProcessData* data) {
    double start = MPI_Wtime();
    
    Particle *particles = (Particle*)malloc(data->N * sizeof(Particle));
    int seed;
    int *seeds = (int*)malloc(data->size * sizeof(int));
    
    if (data->rank == MASTER) {
        srand(time(NULL));
        
        for (int i = 0; i < data->size; i++) {
            seeds[i] = rand();
        }
    }
    
    // Процесс MASTER посылает остальным процессам данные из seeds по одному элементу
    MPI_Scatter(seeds, 1, MPI_UNSIGNED, &seed, 1, MPI_UNSIGNED, MASTER, MPI_COMM_WORLD);
    
    for (int i = 0; i < data->N; i++) {
        particles[i].r = rand_r(&seed) % (data->a * data->b);
        particles[i].x = rand_r(&seed) % data->l;
        particles[i].y = rand_r(&seed) % data->l;
    }
    
    int squareSize = data->l * data->l * data->size;
    int *squares = (int*)malloc(squareSize * sizeof(int));
    
    for (int i = 0; i < squareSize; i++) squares[i] = 0;
    
    MPI_File_delete("data.bin", MPI_INFO_NULL);
    MPI_File binaryFile;
    MPI_File_open(MPI_COMM_WORLD, "data.bin", MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &binaryFile);
    
    for (int i = 0; i < data->N; i++) {
        int x = particles[i].x;
        int y = particles[i].y;
        int r = particles[i].r;
        squares[y * data->l * data->size + x * data->size + r]++;
    }
    
    MPI_Datatype view;
    MPI_Type_vector(data->l, data->l * data->size, data->l * data->a * data->size, MPI_INT, &view);
    MPI_Type_commit(&view);
    
    int xCoord = data->rank % data->a;
    int yCoord = data->rank / data->a;
    int offset = (xCoord * data->l + yCoord * data->a * data->l * data->l) * data->size;
    
    MPI_File_set_view(binaryFile, offset * sizeof(int), MPI_INT, view, "native", MPI_INFO_NULL);
    
    MPI_File_write(binaryFile, squares, squareSize, MPI_INT, MPI_STATUS_IGNORE);
    MPI_Type_free(&view);
    
    MPI_File_close(&binaryFile);
    
    double finish = MPI_Wtime();
    
    if (data->rank == MASTER) {
        double delta = finish - start;
        FILE *file = fopen("stats.txt", "w");
        fprintf(file, "%d %d %d %d %fs\n", data->l, data->a,
                data->b, data->N, delta);
        fclose(file);
    }
    
    free(particles);
    free(seeds);
    free(squares);
}

int main(int argc, char * argv[]) {
    int l = atoi(argv[1]);
    int a = atoi(argv[2]);
    int b = atoi(argv[3]);
    int N = atoi(argv[4]);
    
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    ProcessData data;
    data.l = l;
    data.a = a;
    data.b = b;
    data.N = N;
    data.rank = rank;
    data.size = size;
    generate_points(&data);
    
    MPI_Finalize();
    return 0;
}
