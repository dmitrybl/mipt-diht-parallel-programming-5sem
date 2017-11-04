//
//  main.c
//  Pthreads_mergesort
//
//  Created by Dmitry Belyaev on 04.11.17.
//  Copyright © 2017 Dmitry Belyaev. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <omp.h>

typedef struct merge_sort_struct {
    int size;
    int m;
    int* array;
    int* T;
    int number_of_threads;
    pthread_t* threads_array;
} merge_sort_struct;

int cmpfunc (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

typedef struct merge_struct {
    int* left_array;
    int* right_array;
    int* array;
    int* T;
    int l_size;
    int r_size;
    int m;
    int number_of_threads;
    pthread_t* threads_array;
} merge_struct;

void usual_merge(merge_struct* ctx) {
    int start = 0;
    int L1 = 0;
    int L2 = 0;
    while(L1 < ctx->l_size && L2 < ctx->r_size) {
        if(ctx->left_array[L1] < ctx->right_array[L2]) {
            ctx->T[start] = ctx->left_array[L1];
            start++;
            L1++;
        }
        else {
            ctx->T[start] = ctx->right_array[L2];
            start++;
            L2++;
        }
    }
    
    if(L1 >= ctx->l_size) {
        while(L2 < ctx->r_size) {
            ctx->T[start] = ctx->right_array[L2];
            start++;
            L2++;
        }
    }
    else {
        while(L1 < ctx->l_size) {
            ctx->T[start] = ctx->left_array[L1];
            start++;
            L1++;
        }
    }
    
    int length = ctx->l_size + ctx->r_size;
    memcpy(ctx->array, ctx->T, length * sizeof(int));
}

void usual_merge_sort(merge_sort_struct* ctx) {
    if (ctx->size <= ctx->m) {
        qsort(ctx->array, ctx->size, sizeof(int), cmpfunc);
    } else {
        int mid = ctx->size / 2;
        merge_sort_struct sort_right = {
            .size = ctx->size - mid,
            .m = ctx->m,
            .array = ctx->array + mid,
            .T = ctx->T + mid,
        };
        merge_sort_struct sort_left = {
            .size = mid,
            .m = ctx->m,
            .array = ctx->array,
            .T = ctx->T,
        };
        usual_merge_sort(&sort_left);
        usual_merge_sort(&sort_right);
        merge_struct merge_struct = {
            .l_size = mid,
            .r_size = ctx->size - mid,
            .left_array = ctx->array,
            .right_array = ctx->array + mid,
            .array = ctx->array,
            .T = ctx->T,
            .m = ctx->m,
        };
        usual_merge(&merge_struct);
    }
}

int binarySearch(int* array, int left, int right, int key) {
    int l = left - 1;
    int r = right + 1;
    while(l < r - 1) {
        int m = (r + l) / 2;
        if(array[m] < key) {
            l = m;
        }
        else {
            r = m;
        }
    }
    return r;
}

void* parallel_merge(void* pointer) {
    merge_struct* ctx = (merge_struct*) pointer;
    if (ctx->number_of_threads <= 1 || ctx->l_size + ctx->r_size <= ctx->m) {
        usual_merge(ctx);
    } else {
        int pos = binarySearch(ctx->right_array, 0, ctx->r_size, ctx->left_array[ctx->l_size / 2]);
        
        merge_struct merge_left = {
            .array = ctx->array,
            .T = ctx->T,
            .m = ctx->m,
            .threads_array = ctx->threads_array + 2,
            .number_of_threads = ctx->number_of_threads / 2 - 1,
            .l_size = ctx->l_size / 2 ,
            .r_size = pos,
            .left_array = ctx->left_array,
            .right_array = ctx->right_array,
        };
        merge_struct merge_right = {
            .array = ctx->array + ctx->l_size / 2 + pos,
            .T = ctx->T + ctx->l_size / 2 + pos,
            .m = ctx->m,
            .threads_array = ctx->threads_array + merge_left.number_of_threads + 2,
            .number_of_threads = ctx->number_of_threads - merge_left.number_of_threads - 2,
            .l_size = ctx->l_size - ctx->l_size / 2,
            .r_size = ctx->r_size - pos,
            .left_array = ctx->left_array + ctx->l_size / 2,
            .right_array = ctx->right_array + pos,
        };
        pthread_t thread1 = *(ctx->threads_array);
        pthread_create(&thread1, NULL, parallel_merge, &merge_left);
        pthread_t thread2 = *(ctx->threads_array + 1);
        pthread_create(&thread2, NULL, parallel_merge, &merge_right);
        
        pthread_join(thread2, NULL);
        pthread_join(thread1, NULL);
    }
    return NULL;
}

void* parallel_merge_sort(void* pointer) {
    merge_sort_struct* ctx = (merge_sort_struct*) pointer;
    if (ctx->size <= ctx->m) {
        qsort(ctx->array, ctx->size, sizeof(int), cmpfunc);
    }
    else if (ctx->number_of_threads <= 1) {
        usual_merge_sort(ctx);
    }
    else {
        int mid = ctx->size / 2;
        merge_sort_struct sort_left = {
            .size = mid,
            .m = ctx->m,
            .array = ctx->array,
            .T = ctx->T,
            .threads_array = ctx->threads_array + 2,
            .number_of_threads = ctx->number_of_threads / 2 - 1,
        };
        merge_sort_struct sort_right = {
            .size = ctx->size - mid,
            .m = ctx->m,
            .array = ctx->array + mid,
            .T = ctx->T + mid,
            .threads_array = ctx->threads_array + sort_left.number_of_threads + 2,
            .number_of_threads = ctx->number_of_threads - sort_left.number_of_threads - 2,
        };
        pthread_t thread1 = *(ctx->threads_array);
        pthread_create(&thread1, NULL, parallel_merge_sort, &sort_left);
        pthread_t thread2 = *(ctx->threads_array + 1);
        pthread_create(&thread2, NULL, parallel_merge_sort, &sort_right);
        
        pthread_join(thread2, NULL);
        pthread_join(thread1, NULL);
        merge_struct merge_struct = {
            .l_size = mid,
            .r_size = ctx->size - mid,
            .left_array = ctx->array,
            .right_array = ctx->array + mid,
            .array = ctx->T,
            .T = ctx->T,
            .m = ctx->m,
        };
        parallel_merge(&merge_struct);
        memcpy(ctx->array, merge_struct.array, sizeof(int) * ctx->size);
    }
    return NULL;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int P = atoi(argv[3]);
    
    int* array = (int*)malloc(sizeof(int) * n);
    int* A = (int*)malloc(sizeof(int) * n);
    int* B = (int*)malloc(sizeof(int) * n);
    FILE* file = fopen("data.txt", "w");
    
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        array[i] = rand() % 1000;
        B[i] = array[i];
        fprintf(file, "%d ", array[i]);
    }
    fprintf(file, "\n");
    fclose(file);
    
    pthread_t threads_array[P];
    merge_sort_struct S = {
        .size = n,
        .m = m,
        .array = array,
        .T = A,
        .threads_array = threads_array,
        .number_of_threads = P,
    };
    
    double start = omp_get_wtime();
    qsort(B, n, sizeof(int), cmpfunc);
    double finish = omp_get_wtime();
    double qtime = finish - start;
    printf("Qsort time: %f\n", qtime);
    
    start = omp_get_wtime();
    parallel_merge_sort(&S);
    finish = omp_get_wtime();
    double mtime = finish - start;
    printf("Merge sort time: %f\n", mtime);
    
    file = fopen("data.txt", "a");
    for(int i = 0; i < n; i++) {
        fprintf(file, "%d ", A[i]);
    }
    fclose(file);
    file = fopen("stats.txt", "w");
    fprintf(file, "%f %f %d %d %d", qtime, mtime, n, m, P);
    fclose(file);
    printf("Sorted in data.txt\n");
    free(A);
    free(B);
    free(array);
    return 0;
}
