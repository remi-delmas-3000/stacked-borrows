#ifndef CPROVER_SHADOW_MEMORY_MAP_DEFINED
#define CPROVER_SHADOW_MEMORY_MAP_DEFINED
#include <assert.h>
#include <stdlib.h>

/*
  A shadow memory map allows to map any individual byte of an object manipulated
  by user code to k shadow bytes in shadow memory.
  A shadow map is simply modelled as a map from object IDs to lazily allocated
  shadow objects. The size of a shadow object is k times the size of its source
  object.
  Given a pointer `ptr` to some byte, a pointer to the start of the k shadow
  bytes is obtained by changing the base address of `ptr` and scaling its offset
  by k.
  It is possible to allocate several different shadow maps with different k
  values in a same program.
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
      .map = __CPROVER_allocate(__nof_objects * sizeof(void *), 1)};
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
    __shadow_memory_map->map[id] = shadow_object_ptr;
  }
  return shadow_object_ptr + __shadow_memory_map->shadow_bytes_per_byte *
                                 __CPROVER_POINTER_OFFSET(ptr);
}

#endif