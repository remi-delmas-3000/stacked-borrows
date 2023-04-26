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
  size_t k;
  // pointers to shadow objects
  void **ptrs;
} shadow_map_t;

extern size_t __CPROVER_max_malloc_size;
int __builtin_clzll(unsigned long long);

// computes 2^OBJECT_BITS
#define __nof_objects                                                          \
  ((size_t)(1ULL << __builtin_clzll(__CPROVER_max_malloc_size)))

// Initialises the given shadow memory map
void shadow_map_init(shadow_map_t *smap, size_t k) {
  *smap = (shadow_map_t){
      .k = k, .ptrs = __CPROVER_allocate(__nof_objects * sizeof(void *), 1)};
}

// Returns a pointer to the shadow bytes of the byte pointed to by ptr
void *shadow_map_get(shadow_map_t *smap, void *ptr) {
  size_t id = __CPROVER_POINTER_OBJECT(ptr);
  void *sptr = smap->ptrs[id];
  if (!sptr) {
    sptr = __CPROVER_allocate(smap->k * __CPROVER_OBJECT_SIZE(ptr), 1);
    smap->ptrs[id] = sptr;
  }
  return sptr + smap->k * __CPROVER_POINTER_OFFSET(ptr);
}

#endif
