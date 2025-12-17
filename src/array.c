#include "include/array.h"
#include <string.h>

#define ITEM_COUNT 50
#define ARRAY_INDEX(a, idx) ((a)->item_size * (idx))

struct array *array_new(size_t size) {
  struct array *a = calloc(1, sizeof(*a));
  if (a) {
    a->items = calloc(ITEM_COUNT, size);
    if (!a->items) {
      free(a);
      return NULL;
    }
    a->item_size = size;
    a->capacity = ITEM_COUNT;
    a->length = 0;
  }

  return a;
}

void *_grow_array(struct array *a, size_t new_cap) {
  void **tmp = NULL;
  if (a && new_cap > 0 && new_cap > a->capacity) {
    tmp = realloc(a->items, new_cap * a->item_size);
    if (tmp) {
      memset(((char *)tmp) + a->length, 0,
             (new_cap - a->length) * a->item_size);
      a->items = tmp;
      a->capacity = new_cap;
    }
  }
  return tmp;
}

void *array_push(struct array *a, void *item) {
  void *ptr = NULL;
  if (a && item) {
    if (a->length + 1 >= a->capacity) {
      if (!_grow_array(a, a->capacity * 2)) {
        return NULL;
      }
    }

    memcpy(((char *)a->items) + ARRAY_INDEX(a, a->length), item, a->item_size);
    ptr = ((char *)a->items) + ARRAY_INDEX(a, a->length);
    a->length++;
  }
  return ptr;
}

void *array_pop(struct array *a) {
  void *ptr = NULL;
  if (a) {
    if (a->length > 0) {
      ptr = ((char *)a->items) + ARRAY_INDEX(a, a->length - 1);
      a->length--;
    }
  }
  return ptr;
}

void *array_set(struct array *a, size_t index, void *item) {
  void *ptr = NULL;
  if (a && index > 0 && item) {
    if (index > a->capacity) {
      if (!_grow_array(a, index + 1)) {
        return NULL;
      }
      memcpy(((char *)a->items) + ARRAY_INDEX(a, index), item, a->item_size);
      ptr = ((char *)a->items) + ARRAY_INDEX(a, index);
      a->length = index + 1;
    }
  }
  return ptr;
}

void *array_get(struct array *a, size_t index) {
  if (a && index < a->length && index >= 0) {
    return ((char *)a->items) + ARRAY_INDEX(a, index);
  }
  return NULL;
}

size_t array_size(struct array *a) {
  if (a) {
    return a->length;
  }
  return 0;
}

void array_reset(struct array *a) {
  if (a) {
    memset(a->items, 0, a->capacity * a->item_size);
    a->length = 0;
  }
}

void array_destroy(struct array *a) {
  if (a) {
    if (a->items) {
      free(a->items);
    }
    free(a);
  }
}
