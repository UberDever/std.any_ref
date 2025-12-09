#ifndef __VECTOR3_H__
#define __VECTOR3_H__

#include "std.any_ref/public/std.any_ref/api.h"

typedef struct vector3_t {
  float x, y, z;
} vector3_t;

typedef vector3_t *vector3_ptr_t;

ANY_TYPE_DECLARE(vector3_t, vector3_t);
ANY_TYPE_DECLARE(vector3_ptr_t, vector3_t *);

#endif /* __VECTOR3_H__ */