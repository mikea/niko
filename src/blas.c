#include "niko.h"

#include <cblas.h>

DEF_WORD("blas_gemm", blas_gemm) {
  STATUS_CHECK(stack_len(stack) > 1, "stack underflow: 2 values expected");
  own(array_t) y = stack_pop(stack);
  own(array_t) x = stack_pop(stack);

  STATUS_CHECK(x->r == 2, "rank 2 expected");
  STATUS_CHECK(y->r == 2, "rank 2 expected");

  dim_t m = array_dims(x)[0];
  dim_t k = array_dims(x)[1];
  dim_t n = array_dims(y)[1];

  STATUS_CHECK(k == array_dims(y)[0], "incompatible matrix size");

  dim_t dims[2] = {m, n};

  STATUS_CHECK(y->t == T_F64, "(todo) f64 expected");
  STATUS_CHECK(x->t == T_F64, "(todo) f64 expected");

  array_t* z = array_alloc(T_F64, dims[0] * dims[1], shape_create(2, dims));

  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0, array_data(x), k, array_data(y), n, 0.0,
              array_mut_data(z), n);

  stack_push(stack, z);
  STATUS_OK;
}