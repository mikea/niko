#include "array.h"

array_t* array_alloc(type_t t, size_t n, flags_t f) {
  array_t* a;

  if (__array_data_simd_aligned(t, n)) {
    a    = malloc(sizeof(array_t));
    a->p = aligned_alloc(SIMD_REG_WIDTH_BYTES, SIMD_ALIGN_BYTES(type_sizeof(t, n)));
  } else {
    a    = malloc(sizeof(array_t) + type_sizeof(t, n));
    a->p = (void*)(a + 1);
  }

  a->t     = t;
  a->f     = f;
  a->rc    = 1;
  a->n     = n;
  a->owner = NULL;
  return a;
}

array_t* array_new(type_t t, size_t n, flags_t f, const void* x) {
  array_t* a = array_alloc(t, n, f);
  memcpy(array_mut_data(a), x, type_sizeof(t, n));
  if (t == T_ARR) DO_MUT_ARRAY(a, t_arr, i, p) array_inc_ref(*p);
  return a;
}

array_t* array_new_slice(array_t* x, size_t n, const void* p) {
  array_inc_ref(x);
  array_t* y = malloc(sizeof(array_t));

  y->t       = x->t;
  y->f       = 0;
  y->rc      = 1;
  y->n       = n;
  y->p       = (void*)p;
  y->owner   = x;
  return y;
}

void array_free(array_t* a) {
  if (a->t == T_ARR) {
    DO_ARRAY(a, t_arr, i, p) { array_dec_ref(*p); }
  }
  if (a->owner) array_dec_ref(a->owner);
  else if (__array_data_simd_aligned(a->t, a->n)) free(a->p);
  free(a);
}
