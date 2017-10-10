#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <string.h>

int cmpfunc (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
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

void merge(int* array, int l1, int r1, int l2, int r2, int m, int* A, int left3) {
    int n1 = r1 - l1 + 1;
    int n2 = r2 - l2 + 1;
    
    if(n1 + n2 <= m) {
        int start = left3;
        int L1 = l1;
        int L2 = l2;
        while(L1 <= r1 && L2 <= r2) {
            if(array[L1] < array[L2]) {
                A[start] = array[L1];
                start++;
                L1++;
            }
            else {
                A[start] = array[L2];
                start++;
                L2++;
            }
        }
        
        if(L1 > r1) {
            while(L2 <= r2) {
                A[start] = array[L2];
                start++;
                L2++;
            }
        }
        else {
            while(L1 <= r1) {
                A[start] = array[L1];
                start++;
                L1++;
            }
        }
    }
    else {
            if (n1 < n2) {
                swap(&l1, &l2);
                swap(&r1, &r2);
                swap(&n1, &n2);
            }
        
            if (n1 == 0) {
                return;
            }
            int mid1 = (l1 + r1) / 2;
            int mid2 = binarySearch(array, l2, r2, array[mid1]);
            int mid3 = left3 + (mid1 - l1) + (mid2 - l2);
            A[mid3] = array[mid1];
            #pragma omp task
                merge(array, l1, mid1 - 1, l2, mid2 - 1, m, A, left3);
            #pragma omp task
                merge(array, mid1 + 1, r1, mid2, r2, m, A, mid3 + 1);
            #pragma omp taskwait
    }
}

void parallel_merge_sort(int* array, int left, int right, int m, int* A) {
    if(right - left < m) {
        qsort(array + left, right - left + 1, sizeof(int), cmpfunc);
    }
    else {
        int mid = left + (right - left) / 2;
        #pragma omp task
            parallel_merge_sort(array, left, mid, m, A);
        #pragma omp task
            parallel_merge_sort(array, mid + 1, right, m, A);
        #pragma omp taskwait
        merge(array, left, mid, mid + 1, right, m, A, left);
        memcpy(array + left, A + left, sizeof(int) * (right - left + 1));
    }
}

int main(int argc, const char * argv[]) {
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    int P = atoi(argv[3]);
    
    int* array = (int*)malloc(sizeof(int) * n);
    int* A = (int*)malloc(sizeof(int) * n);
    int* B = (int*)malloc(sizeof(int) * n);
    
    FILE* file = fopen("data.txt", "w");

    srand(time(NULL));
    for(int i = 0; i < n; i++) {
        array[i] = rand() % 1000000;
        B[i] = array[i];
        fprintf(file, "%d ", array[i]);
    }
    
    fprintf(file, "\n");
    fclose(file);
    
    for(int i = 0; i < n; i++) {
        A[i] = 0;
    }
    
    omp_set_num_threads(P);
    double start = omp_get_wtime();
    qsort(B, n, sizeof(int), cmpfunc);
    double finish = omp_get_wtime();
    double qtime = finish - start;
    printf("Qsort time: %f\n", qtime);
    
    start = omp_get_wtime();
    #pragma omp parallel
        {
        #pragma omp single
            {
                parallel_merge_sort(array, 0, n - 1, m, A);
            }
        }
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
