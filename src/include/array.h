#ifndef ARRAY_H__
#define ARRAY_H__

#include <stdlib.h>

struct array {
  void *items;
  size_t item_size;
  size_t capacity;
  size_t length;
};

struct array *array_new(size_t size);
void array_reset(struct array *a);
void array_destroy(struct array *a);
void *array_pop(struct array *a);
void *array_push(struct array *a, void *item);
void *array_set(struct array *a, size_t index, void *item);
void *array_get(struct array *a, size_t index);
size_t array_size(struct array *a);

#endif /* ARRAY_H__ */
