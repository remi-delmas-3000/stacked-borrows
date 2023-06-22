#ifndef CPROVER_SHADOW_MAP_DEFINED
#define CPROVER_SHADOW_MAP_DEFINED
#include <assert.h>
#include <stdlib.h>

/*
  A shadow map allows to map any individual byte of an object manipulated
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
  // nof shadow bytes per byte
  size_t shadow_bytes_per_byte;
  // pointers to shadow objects
  void **ptrs;
} shadow_map_t;

extern size_t __CPROVER_max_malloc_size;
int __builtin_clzll(unsigned long long);

// computes 2^OBJECT_BITS
#define __nof_objects                                                          \
  ((size_t)(1ULL << __builtin_clzll(__CPROVER_max_malloc_size)))

// Initialises the given shadow memory map
void shadow_map_init(shadow_map_t *smap, size_t shadow_bytes_per_byte) {
  __CPROVER_assert(1 == shadow_bytes_per_byte || 2 == shadow_bytes_per_byte ||
                       4 == shadow_bytes_per_byte || 8 == shadow_bytes_per_byte,
                   "shadow_bytes_per_byte must be in {1, 2, 4, 8}");
  *smap = (shadow_map_t){
      .shadow_bytes_per_byte = shadow_bytes_per_byte,
      .ptrs = __CPROVER_allocate(__nof_objects * sizeof(void *), 1)};
}

// Returns a pointer to the shadow bytes of the byte pointed to by ptr
void *shadow_map_get(shadow_map_t *smap, void *ptr) {
  __CPROVER_size_t id = __CPROVER_POINTER_OBJECT(ptr);
  __CPROVER_size_t size = __CPROVER_OBJECT_SIZE(ptr);
  __CPROVER_size_t offset = __CPROVER_POINTER_OFFSET(ptr);

  size_t max_size = SIZE_MAX / smap->shadow_bytes_per_byte;
  __CPROVER_assert(size <= max_size, " no overflow on size scaling");
  __CPROVER_assert(offset <= max_size, " no overflow on offset scaling");

  void *sptr = smap->ptrs[id];
  if (!sptr) {
    sptr = __CPROVER_allocate(
        smap->shadow_bytes_per_byte * __CPROVER_OBJECT_SIZE(ptr), 1);
    smap->ptrs[id] = sptr;
  }
  return sptr + (smap->shadow_bytes_per_byte * __CPROVER_POINTER_OFFSET(ptr));
}

#endif
