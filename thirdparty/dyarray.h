#include <assert.h>
#include <stdlib.h>

#define INIT_CAP 2
#define CAP_GROWTH_FACTOR 2
#define ALLOC_ERR "failed to allocate more memory for dynamic array"

#define dyarray_type(type)                                                     \
  struct {                                                                     \
    size_t size;                                                               \
    size_t cap;                                                                \
    type *items;                                                               \
  }

#define dyarray_reserve(arr, expected_cap)                                     \
  do {                                                                         \
    if ((arr)->cap < (expected_cap)) {                                         \
      if ((arr)->cap == 0) {                                                   \
        (arr)->cap = INIT_CAP;                                                 \
      }                                                                        \
      while ((arr)->cap < (expected_cap)) {                                    \
        (arr)->cap *= CAP_GROWTH_FACTOR;                                       \
      }                                                                        \
    }                                                                          \
    (arr)->items =                                                             \
        realloc((arr)->items, ((arr)->cap * sizeof(*(arr)->items)));           \
    assert((arr)->items != NULL && ALLOC_ERR);                                 \
  } while (0)

#define dyarray_push(arr, item)                                                \
  do {                                                                         \
    dyarray_reserve((arr), (arr)->size + 1);                                   \
    (arr)->items[((arr)->size++)] = (item);                                    \
  } while (0)

#define dyarray_pop(arr)                                                       \
  do {                                                                         \
    if ((arr)->size > 0) {                                                     \
      (arr)->size--;                                                           \
    }                                                                          \
  } while (0)

#define dyarray_free(arr) free((arr).items)

#define dyarray_shift(arr)                                                     \
  do {                                                                         \
    if ((arr)->size > 0) {                                                     \
      for (size_t i = 1; i < (arr)->size; ++i) {                               \
        (arr)->items[i - 1] = (arr)->items[i];                                 \
      }                                                                        \
      (arr)->size--;                                                           \
    }                                                                          \
  } while (0)
