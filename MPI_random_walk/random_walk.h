#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <malloc.h>

#define MASTER 0
#define OVERLAP 5
#define ITERS_PER_EXCHANGE 1000

typedef struct {
    int l;
    int a;
    int b;
    int n;
    int N;
    float pl;
    float pr;
    float pu;
    float pd;
    float prob[4];
    int rank, size;
    int* result;
} Data;

typedef struct {
    int i;
    int x;
    int y;
} Point;

typedef struct {
    Point* array;
    int size;
    int max_size;
} Vector;

typedef struct {
    Vector points;
    int n;
} thread_data;

void create_vector(Vector* v, int size);
void vector_push(Vector* v, Point element);
void vector_set(Vector* v, Point element, int position);
void vector_alloc(Vector* v, int position);
void vector_unite(Vector* v1, Vector* v2);

int step(thread_data* t_data, Data* data);

void send_vector(int to, Vector* points);
Vector receive_vector(int from);

void communicate_for(int prev, int next, Data* data, Vector* send, Vector* points);
void random_walk(void* arg);

int scatter_seeds(int rank, int size);
int* gather_counts(int rank, int size, int count);
