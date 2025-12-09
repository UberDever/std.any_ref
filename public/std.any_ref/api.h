#ifndef __LIB_UTIL_ANY_REF_API_H__
#define __LIB_UTIL_ANY_REF_API_H__

#include <stddef.h>

#ifndef ANY_REF_INLINE_BYTES
#define ANY_REF_INLINE_BYTES 32u
#endif

typedef struct any_type {
  const char *name;
  size_t size;
} any_type;

typedef union any_storage {
  void *ptr;
  unsigned char bytes[ANY_REF_INLINE_BYTES];
} any_storage;

typedef struct any_ref {
  const any_type *type;
  any_storage storage;
  size_t size;
  int is_inline;
} any_ref_t;

#define ANY_TYPE_DECLARE(TAG, T)                                               \
  static const any_type any_type_##TAG = {#T, sizeof(T)}

#define ANY_TYPE(TAG) (&any_type_##TAG)

#define ANY_FROM(value, TAG) any_ref_from_raw(&(value), ANY_TYPE(TAG))

#define ANY_REF_IS(TAG, ref) any_ref_is_type(&(ref), ANY_TYPE(TAG))

#define ANY_REF_AS(TAG, ref) *(TAG *)any_ref_as_raw(&(ref), ANY_TYPE(TAG))

#define ANY_REF_AS_CONST(TAG, ref)                                             \
  *(const TAG *)any_ref_as_raw_const(&(ref), ANY_TYPE(TAG))

void *any_ref_data(any_ref_t *r);
const void *any_ref_data_const(const any_ref_t *r);

int any_ref_is_type(const any_ref_t *r, const any_type *t);
void *any_ref_as_raw(any_ref_t *r, const any_type *expected);
const void *any_ref_as_raw_const(const any_ref_t *r, const any_type *expected);
any_ref_t any_ref_from_raw(const void *src, const any_type *type);

#endif // __INTERNAL_UTIL_ANY_REF_API_H__
