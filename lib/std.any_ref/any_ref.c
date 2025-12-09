#include "api.h"
#include <string.h>

any_ref_t any_ref_from_raw(const void *src, const any_type *type) {
  any_ref_t r;
  r.type = type;
  r.size = type->size;

  memset(&r.storage, 0, sizeof(r.storage));
  if (type->size <= ANY_REF_INLINE_BYTES) {
    memcpy(r.storage.bytes, src, type->size);
    r.is_inline = 1;
  } else {
    r.storage.ptr = (void *)src;
    r.is_inline = 0;
  }

  return r;
}

void *any_ref_data(any_ref_t *r) {
  return r->is_inline ? (void *)r->storage.bytes : r->storage.ptr;
}

const void *any_ref_data_const(const any_ref_t *r) {
  return r->is_inline ? (const void *)r->storage.bytes : r->storage.ptr;
}

int any_ref_is_type(const any_ref_t *r, const any_type *t) {
  return r->type == t;
}

void *any_ref_as_raw(any_ref_t *r, const any_type *expected) {
  if (!any_ref_is_type(r, expected)) { return NULL; }
  return any_ref_data(r);
}

const void *any_ref_as_raw_const(const any_ref_t *r, const any_type *expected) {
  if (!any_ref_is_type(r, expected)) { return NULL; }
  return any_ref_data_const(r);
}