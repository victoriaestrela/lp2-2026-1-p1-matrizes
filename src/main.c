#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "matrix_multiply.h"

#define TOLERANCE 1e-6

/* preenche a matriz com valores deterministicos a partir de uma semente */
static void fill_matrix(double *m, int n, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            m[i * n + j] = (double)(rand() % 1000) / 100.0;
        }
    }
}

/* calcula a soma de todos os elementos da matriz (checksum para verificacao rapida) */
static double checksum(const double *c, int n) {
    double sum = 0.0;
    for (int i = 0; i < n * n; i++) {
        sum += c[i];
    }
    return sum;
}

/* compara duas matrizes elemento a elemento com tolerancia para ponto flutuante */
static int compare_matrices(const double *c_seq, const double *c_par, int n) {
    for (int i = 0; i < n * n; i++) {
        if (fabs(c_seq[i] - c_par[i]) > TOLERANCE) {
            printf("  Diferenca no elemento [%d]: seq=%.10f  par=%.10f\n",
                   i, c_seq[i], c_par[i]);
            return 0;
        }
    }
    return 1;
}

/* converte diferenca entre dois timespec para segundos (double) */
static double elapsed_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)
           + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <n> <num_threads> [num_runs]\n\n", argv[0]);
        fprintf(stderr, "  n           : tamanho da matriz n x n (ex: 1200)\n");
        fprintf(stderr, "  num_threads : numero de threads (ex: 4)\n");
        fprintf(stderr, "  num_runs    : repeticoes cronometradas (padrao: 1)\n");
        fprintf(stderr, "                use >= 5 para media confiavel (um aquecimento\n");
        fprintf(stderr, "                extra e executado antes, fora da contagem)\n\n");
        fprintf(stderr, "Exemplos:\n");
        fprintf(stderr, "  %s 1200 4              # verifica corretude\n", argv[0]);
        fprintf(stderr, "  %s 1200 4 5            # 5 medicoes (+ 1 aquecimento)\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int num_runs = (argc >= 4) ? atoi(argv[3]) : 1;

    if (n <= 0 || num_threads <= 0 || num_runs <= 0) {
        fprintf(stderr, "Erro: parametros devem ser positivos.\n");
        return 1;
    }

    size_t size = (size_t)n * (size_t)n * sizeof(double);
    double *a = (double *)malloc(size);
    double *b = (double *)malloc(size);
    double *c_seq = (double *)malloc(size);
    double *c_par = (double *)malloc(size);

    if (!a || !b || !c_seq || !c_par) {
        fprintf(stderr, "Erro: falha na alocacao de memoria.\n");
        free(a); free(b); free(c_seq); free(c_par);
        return 1;
    }

    /* preenchimento das matrizes A e B em codigo (fora do cronometro) */
    fill_matrix(a, n, 42);
    fill_matrix(b, n, 123);

    printf("=== P1 - Multiplicacao de Matrizes (mapa por linhas) ===\n");
    printf("Matrizes %d x %d  |  memoria: ~%.1f MB\n",
           n, n, 3.0 * size / (1024.0 * 1024.0));
    printf("Threads: %d  |  repeticoes: %d\n\n", num_threads, num_runs);

    /* --- VERIFICACAO DE CORRETUDE --- */
    multiply_seq(a, b, c_seq, n);
    multiply_par(a, b, c_par, n, num_threads);

    if (compare_matrices(c_seq, c_par, n)) {
        printf("VERIFICACAO: OK\n");
        printf("  checksum sequencial = %.6f\n", checksum(c_seq, n));
        printf("  checksum paralelo   = %.6f\n", checksum(c_par, n));
    } else {
        printf("VERIFICACAO: FALHA\n");
        free(a); free(b); free(c_seq); free(c_par);
        return 1;
    }

    /* --- BENCHMARK --- */
    if (num_runs <= 0) {
        free(a); free(b); free(c_seq); free(c_par);
        return 0;
    }

    struct timespec t0, t1;
    double *seq_times = malloc((size_t)num_runs * sizeof(double));
    double *par_times = malloc((size_t)num_runs * sizeof(double));

    if (!seq_times || !par_times) {
        fprintf(stderr, "Erro: falha ao alocar arrays de tempo.\n");
        free(a); free(b); free(c_seq); free(c_par);
        free(seq_times); free(par_times);
        return 1;
    }

    if (num_runs >= 5) {
        printf("\n=== BENCHMARK (%d medicoes + 1 aquecimento) ===\n\n", num_runs);
    } else {
        printf("\n=== MEDICAO UNICA (%d repeticoes) ===\n\n", num_runs);
    }

    /* aquecimento: executa ambas as versoes uma vez (resultado descartado) */
    multiply_seq(a, b, c_seq, n);
    multiply_par(a, b, c_par, n, num_threads);

    /* repeticoes cronometradas da versao sequencial */
    for (int r = 0; r < num_runs; r++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        multiply_seq(a, b, c_seq, n);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        seq_times[r] = elapsed_sec(t0, t1);
    }

    /* repeticoes cronometradas da versao paralela */
    for (int r = 0; r < num_runs; r++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        multiply_par(a, b, c_par, n, num_threads);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        par_times[r] = elapsed_sec(t0, t1);
    }

    /* calcula medias */
    double seq_sum = 0.0, par_sum = 0.0;
    for (int r = 0; r < num_runs; r++) {
        seq_sum += seq_times[r];
        par_sum += par_times[r];
    }
    double seq_avg = seq_sum / num_runs;
    double par_avg = par_sum / num_runs;
    double speedup = (par_avg > 0.0) ? seq_avg / par_avg : 0.0;
    double efficiency = speedup / num_threads;

    printf("Medias (%d execucoes):\n", num_runs);
    printf("  T_seq      = %.6f s\n", seq_avg);
    printf("  T_par (%2dT) = %.6f s\n", num_threads, par_avg);
    printf("  Speedup    = %.4fx\n", speedup);
    printf("  Eficiencia = %.4f\n\n", efficiency);

    /* exibe repeticoes individuais */
    printf("Repeticoes sequenciais (s):");
    for (int r = 0; r < num_runs; r++) printf(" %.4f", seq_times[r]);

    printf("\nRepeticoes paralelas   (s):");
    for (int r = 0; r < num_runs; r++) printf(" %.4f", par_times[r]);
    printf("\n");

    free(seq_times);
    free(par_times);
    free(a);
    free(b);
    free(c_seq);
    free(c_par);

    return 0;
}
