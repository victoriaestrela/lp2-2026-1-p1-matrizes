#include <pthread.h>
#include <stdlib.h>
#include "matrix_multiply.h"

/* argumentos passados para cada thread trabalhadora */
typedef struct {
    const double *a;
    const double *b;
    double *c;
    int n;
    int start_row;
    int end_row;
} thread_arg_t;

/* funcao executada por cada thread: calcula as linhas [start_row, end_row) de C */
static void *worker(void *arg) {
    thread_arg_t *t_arg = (thread_arg_t *)arg;
    const double *a = t_arg->a;
    const double *b = t_arg->b;
    double *c = t_arg->c;
    int n = t_arg->n;

    for (int i = t_arg->start_row; i < t_arg->end_row; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            c[i * n + j] = sum;
        }
    }

    return NULL;
}

/* versao sequencial: triplo laco aninhado, O(n^3) */
void multiply_seq(const double *a, const double *b, double *c, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

/* versao paralela com pthreads: cada thread calcula um bloco contiguo de linhas.
   escrita em posicoes disjuntas -> sem merge, sem estado compartilhado. */
void multiply_par(const double *a, const double *b, double *c, int n, int num_threads) {
    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    thread_arg_t *args = malloc((size_t)num_threads * sizeof(thread_arg_t));

    int rows_per_thread = n / num_threads;
    int extra = n % num_threads;
    int start = 0;

    for (int t = 0; t < num_threads; t++) {
        args[t].a = a;
        args[t].b = b;
        args[t].c = c;
        args[t].n = n;
        args[t].start_row = start;
        int rows = rows_per_thread + (t < extra ? 1 : 0);
        args[t].end_row = start + rows;
        start += rows;

        pthread_create(&threads[t], NULL, worker, &args[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    free(threads);
    free(args);
}
