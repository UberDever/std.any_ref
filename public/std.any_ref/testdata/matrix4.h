#ifndef __MATRIX4_H__
#define __MATRIX4_H__

#include "std.any_ref/public/std.any_ref/api.h"

typedef struct matrix4_t {
  float m[4][4];
} matrix4_t;

typedef matrix4_t *matrix4_ptr_t;

ANY_TYPE_DECLARE(matrix4_t, matrix4_t);
ANY_TYPE_DECLARE(matrix4_ptr_t, matrix4_t *);

#endif /* __MATRIX4_H__ */