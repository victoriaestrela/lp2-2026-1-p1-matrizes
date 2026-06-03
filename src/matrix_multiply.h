#ifndef MATRIX_MULTIPLY_H
#define MATRIX_MULTIPLY_H

void multiply_seq(const double *a, const double *b, double *c, int n);

void multiply_par(const double *a, const double *b, double *c, int n, int num_threads);

#endif
