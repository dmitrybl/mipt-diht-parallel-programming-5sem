#include <malloc.h>
#include "random_walk.h"

void send_vector(int to, Vector* points) {
    MPI_Send(&points->size, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
    MPI_Send(points->array, points->size * sizeof(Point), MPI_BYTE, to, 0, MPI_COMM_WORLD);
}

Vector receive_vector(int from) {
    int n;
    MPI_Recv(&n, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    Point* buffer = malloc(n * sizeof(Point));
    
    MPI_Recv(buffer, n * sizeof(Point), MPI_BYTE, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    Vector vector;
    vector.array = buffer;
    vector.size = n;
    vector.max_size = n;
    return vector;
}

int scatter_seeds(int rank, int size) {
    int* buffer = NULL;
    if (rank == MASTER) {
        srand(time(NULL));
        buffer = calloc(size, sizeof(int));
        for (int i = 0; i < size; i++) {
            buffer[i] = rand();
        }
    }
    int seed;
    MPI_Scatter(buffer, 1, MPI_INT, &seed, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    if (buffer != NULL) {
        free(buffer);
    }
    return seed;
}

int* gather_counts(int rank, int size, int count) {
    int* buffer = NULL;
    if (rank == MASTER) {
        buffer = calloc(size, sizeof(int));
    }
    MPI_Gather(&count, 1, MPI_INT, buffer, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    return buffer;
}

void create_vector(Vector* v, int size) {
    v->array = malloc(size * sizeof(Point));
    v->max_size = size;
    v->size = 0;
}

void vector_push(Vector* v, Point element) {
    if (v->size >= v->max_size) {
        v->max_size *= 2;
        v->array = realloc(v->array, v->max_size * sizeof(Point));
    }
    v->array[v->size] = element;
    v->size++;
}

void vector_alloc(Vector* v, int position) {
    if (position >= v->max_size) {
        v->max_size = position + 1;
        v->array = realloc(v->array, v->max_size * sizeof(Point));
    }
    if (position + 1 > v->size) {
        v->size = position + 1;
    }
}

void vector_set(Vector* v, Point element, int position) {
    vector_alloc(v, position);
    v->array[position] = element;
}

int mod(int x, int y) {
    if ((x % y) >= 0) {
        return x % y;
    }
    else {
        return y + x % y;
    }
}

int step(thread_data* t_data, Data* data){
    int nodes = 0;
    for (int i = 0; i < t_data->points.size; i++) {
        Point* point = &t_data->points.array[i];

        if (point->i == 0) continue;
        
        if (point->i != 1) {
            --point->i;
            nodes = 1;
        } else {
            continue;
        }
        float p = (float) rand() / RAND_MAX;

        if (p <= data->prob[0]) {
            point->x -= 1;
        } else if (p <= data->prob[1]) {
            point->x += 1;
        } else if (p <= data->prob[2]) {
            point->y += 1;
        } else if (p <= data->prob[3]) {
            point->y -= 1;
        }
    }
    return nodes;
}

int push_data_to_array(Data* data, Point* point, Vector* send, int overlap) {
    if (point->x < overlap * (-1)) {
        point->x += data->l;
        vector_push(&send[0], *point);
        point->i = 0;
    } else if (point->x >= data->l + overlap) {
        point->x -= data->l;
        vector_push(&send[1], *point);
        point->i = 0;
    } else if (point->y < overlap * (-1)) {
        point->y += data->l;
        vector_push(&send[2], *point);
        point->i = 0;
    } else if (point->y >= data->l + overlap) {
        point->y -= data->l;
        vector_push(&send[3], *point);
        point->i = 0;
    }
}

void vector_unite(Vector* v1, Vector* v2) {
    if (v2->size != 0) {
        int last_elem_in_v1 = 0;
        int index = 0;
        while (index < v2->size && last_elem_in_v1 < v1->size) {
            while (last_elem_in_v1 < v1->size && v1->array[last_elem_in_v1].i != 0) {
                last_elem_in_v1++;
            }
            if (last_elem_in_v1 < v1->size) {
                v1->array[last_elem_in_v1] = v2->array[index];
                index++;
            }
        }

        while(index < v2->size) {
            vector_push(v1, v2->array[index]);
            index++;
        }
    }
}

void exchange(Data* data, thread_data* t_data, int overlap) {
    Vector send[4];
    for (int i = 0; i < 4; i++) {
        create_vector(&send[i], 10);
    }

    for (int i = 0; i < t_data->points.size; i++) {
        Point* point = &t_data->points.array[i];
        push_data_to_array(data, point, send, overlap);
    }


    int up = mod(data->rank - data->a, data->size);
    int down = mod(data->rank + data->a, data->size);
    int left = mod(data->rank - 1,data->a) + (data->rank / data->a) * data->a;
    int right = mod(data->rank + 1, data->a) + (data->rank / data->a) * data->a;
    
    communicate_for(left, right, data, &send[0], &t_data->points);
    communicate_for(right, left, data, &send[1], &t_data->points);
    communicate_for(up, down, data, &send[2], &t_data->points);
    communicate_for(down, up, data, &send[3], &t_data->points);
}

void communicate_for(int prev, int next, Data* data, Vector* send, Vector* points) {
    int c = data->rank % 2 != next % 2 ? 1 : data->a;
    
    if ((data->rank / c) % 2 == 0) {
        send_vector(next, send);
        Vector buffer = receive_vector(prev);
        vector_unite(points, &buffer);
        Vector* p_buffer = &buffer;
        free(p_buffer->array);
    } else {
        Vector buffer = receive_vector(prev);
        vector_unite(points, &buffer);
        Vector* p_buffer = &buffer;
        free(p_buffer->array);
        send_vector(next, send);
    }
}

void random_walk(void* arg) {
    Data* data = (Data*) arg;

    int seed = scatter_seeds(data->rank, data->size);
    srand(seed);

    thread_data t_data;
    create_vector(&t_data.points, data->N);
    t_data.points.size = data->N;
    for (int j = 0; j < data->N; j++) {
        t_data.points.array[j].i = data->n + 1;
        t_data.points.array[j].x = rand() % data->l;
        t_data.points.array[j].y = rand() % data->l;
    }

    for (int i = 0; i < data->n / ITERS_PER_EXCHANGE + 1; i++) {
        for (int j = 0; j < ITERS_PER_EXCHANGE; j++) {
            if (!step(&t_data, data)) {
                break;
            }
        }
        exchange(data, &t_data, OVERLAP);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < data->a + data->b + 1; i++) {
        exchange(data, &t_data, 0);
    }

    int count = 0;
    for (int i = 0; i < t_data.points.size; i++) {
        if (t_data.points.array[i].i != 0) {
            count++;
        }
    }

    int* result = gather_counts(data->rank, data->size, count);

    data->result = result;

    free((&t_data.points)->array);
}
