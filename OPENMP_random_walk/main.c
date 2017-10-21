//
//  main.c
//  OPENMP_random_walk
//
//  Created by Dmitry Belyaev on 20.10.17.
//  Copyright © 2017 Dmitry Belyaev. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>

int main(int argc, const char * argv[]) {
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int x = atoi(argv[3]);
    int N = atoi(argv[4]);
    float p = atof(argv[5]);
    int P = atoi(argv[6]);
    
    unsigned int seed = clock();
    int totalTime = 0;
    int rightPointsCount = 0;
    omp_set_num_threads(P);
    double start = omp_get_wtime();
    
#pragma omp parallel
    {
#pragma omp for reduction(+: rightPointsCount, totalTime)
        for(int i = 0; i < N; i++) {
            int current = x;
            int time = 0;
            while(current != a && current != b) {
                int random = rand_r(&seed);
                if(random < RAND_MAX * p) {
                    current++;
                }
                else {
                    current--;
                }
                time++;
            }
            
            if(current == b) {
                rightPointsCount++;
            }
            
            totalTime += time;
        }
    }
    
    double finish = omp_get_wtime();
    double delta = finish - start;
    FILE* file = fopen("stats.txt", "w");
    fprintf(file, "%lf %lf %lfs %d %d %d %d %f %d\n", (double)rightPointsCount / N,
            (double)totalTime / N, delta, a,b,x,N,p,P);
    fclose(file);
    printf("Parallel: %fs\n", delta);
    printf("b: %lf\n", (double)rightPointsCount / N);
    printf("S: %lf\n", (double)totalTime / N);		
    return 0;
}
