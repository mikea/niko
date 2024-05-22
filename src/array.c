#include "array.h"

array_t* array_alloc(type_t t, size_t n, shape_t s) {
  assert(shape_len(s) == n);

  array_t* a;

  if (__array_data_simd_aligned(t, n)) {
    a    = malloc(sizeof(array_t) + dims_sizeof(s.r));
    a->p = aligned_alloc(SIMD_REG_WIDTH_BYTES, SIMD_ALIGN_BYTES(type_sizeof(t, n)));
  } else {
    a    = malloc(sizeof(array_t) + dims_sizeof(s.r) + type_sizeof(t, n));
    a->p = (void*)(a + 1) + dims_sizeof(s.r);
  }

  a->t     = t;
  a->f     = 0;
  a->rc    = 1;
  a->n     = n;
  a->r     = s.r;
  a->owner = NULL;
  memcpy(a + 1, s.d, dims_sizeof(s.r));
  return a;
}

void array_free(array_t* a) {
  if (a->t == T_ARR) {
    DO_ARRAY(a, t_arr, i, p) { array_free(*p); }
  }
  if (a->owner) array_dec_ref(a->owner);
  else if (__array_data_simd_aligned(a->t, a->n)) free(a->p);
  free(a);
}