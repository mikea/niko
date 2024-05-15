#include "array.h"

void array_free(array_t* a) {
  if (a->t == T_ARR) {
    DO_ARRAY(a, t_arr, i, p) {
        array_free(*p);
    }
  }
  if (a->owner) array_dec_ref(a->owner);
  else if (__array_data_simd_aligned(a->t, a->n)) free(a->p);
  free(a);
}