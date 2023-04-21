#ifndef CPROVER_SHADOW_MEMORY_MAP_DEFINED
#define CPROVER_SHADOW_MEMORY_MAP_DEFINED
#include <assert.h>
#include <stdlib.h>

/*
  An emulation of shadow memory.

  A global map associates k bytes of shadow data to any individual byte of
  user-space objects (local or dynamic).

  A instance
  ```
  __CPROVER_shadow_memory_map_t __shadow_map;
  ```
  Is a map from object IDs to (lazily allocated) shadow objects pointers.

  The shadow object for some object pointed to by `ptr` is of size
  `k * __CPROVER_OBJECT_SIZE(ptr)` where `k` is user defined.

  A pointer to the k shadow bytes for the byte pointed to by `ptr` is obtained
  by rewriting `ptr` to:

  ```
  __shadow_map[__CPROVER_POINTER_OBJECT(ptr)] + k *
  __CPROVER_POINTER_OFFSET(ptr)
  ```
  It is possible to have several different shadow maps with different k values.
*/


typedef struct {
  size_t shadow_bytes_per_byte;
  // array of void * pointers to shadow objects
  void **map;
} __CPROVER_shadow_memory_map_t;

extern size_t __CPROVER_max_malloc_size;
int __builtin_clzll(unsigned long long);

// computes 2^OBJECT_BITS
#define __nof_objects                                                          \
  ((size_t)(1ULL << __builtin_clzll(__CPROVER_max_malloc_size)))

// Initialises the given shadow memory map
void __CPROVER_shadow_memory_map_init(
    __CPROVER_shadow_memory_map_t *__shadow_memory_map,
    size_t shadow_bytes_per_byte) {
  *__shadow_memory_map = (__CPROVER_shadow_memory_map_t){
      .shadow_bytes_per_byte = shadow_bytes_per_byte,
      .map = __CPROVER_allocate( * sizeof(void *), 1)};
  __CPROVER_array_set(__shadow_memory_map->map, 0);
}

// Returns a pointer to the shadow bytes of the byte pointed to by ptr
void *__CPROVER_shadow_memory_map_get(
    __CPROVER_shadow_memory_map_t *__shadow_memory_map, void *ptr) {
  size_t id = __CPROVER_POINTER_OBJECT(ptr);
  void *shadow_object_ptr = __shadow_memory_map->map[id];
  if (!shadow_object_ptr) {
    shadow_object_ptr = __CPROVER_allocate(
        __shadow_memory_map->shadow_bytes_per_byte * __CPROVER_OBJECT_SIZE(ptr),
        1);
    __CPROVER_array_set(shadow_object_ptr, 0);
    __shadow_memory_map->map[id] = shadow_object_ptr;
  }
  return shadow_object_ptr + __shadow_memory_map->shadow_bytes_per_byte *
                                 __CPROVER_POINTER_OFFSET(ptr);
}

#endif