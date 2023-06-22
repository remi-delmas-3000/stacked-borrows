// Covers the basic definitions of the paper
// "Stacked Borrows: An Aliasing Model for Rust"
// until section 3.5 no optimisations.
#ifndef STACKED_BORROWS_DEFINED
#define STACKED_BORROWS_DEFINED

// analyse with --slice-formula and minisat
// remoarks
#include "shadow_map_mult.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// use symbolic sizes for maps and stacks
bool SB_SYMSIZE = true;

// maximum stack size
size_t SB_MAX_STACK_SIZE = 32;

// Borrow kind
typedef uint8_t sb_kind_t;
// &mut x
const sb_kind_t SB_UNIQUE = 0x1;
// &x
const sb_kind_t SB_SHARED_RO = 0x2;
// *mut x
const sb_kind_t SB_SHARED_RW = 0x4;
// disabled &mut x
const sb_kind_t SB_DISABLED = 0x4;

// Borrow ID type
// We track at most 128 borrows in the program
typedef int8_t sb_id_t;

// Borrow ID used for raw pointers
const sb_id_t __sb_id_bottom = -1;

// Generates a stream of unique borrow IDs
sb_id_t __sb_id_fresh = 0;

// Returns a fresh borrow ID
sb_id_t sb_id_fresh() {
  assert(__sb_id_fresh < UINT16_MAX);
  sb_id_t res = __sb_id_fresh;
  __sb_id_fresh++;
  return res;
}

// Representation of a borrow stack item
typedef struct {
  sb_kind_t kind;
  sb_id_t id;
} sb_item_t;

// A stack of borrow items
typedef struct {
  // Index of the next free slot in elems
  int8_t top;
  // An array of borrow items
  sb_item_t *elems;
} sb_stack_t;

size_t nondet_size_t();

// returns size if symbolic is false, a nondet constrained to
// be at least size otherwise
size_t __init_size(bool symbolic, size_t size) {
  assert(size <= UINT8_MAX);
  if (!symbolic)
    return size;
  size_t result = nondet_size_t();
  // this should ensure that CBMC treats the array as symbolic
  // that that field sensitivity does not kick-in
  __CPROVER_assume(result == size);
  return result;
}

// Creates a fresh borrow stack
sb_stack_t *sb_stack_create() {
  sb_stack_t *stack = __CPROVER_allocate(sizeof(*stack), 1);
  *stack = (sb_stack_t){
      .top = 0,
      .elems = __CPROVER_allocate(
          __init_size(SB_SYMSIZE, sizeof(sb_item_t) * SB_MAX_STACK_SIZE), 1)};
  return stack;
}

void sb_stack_push(sb_stack_t *stack, sb_kind_t kind, sb_id_t id) {
  assert(stack->top < SB_MAX_STACK_SIZE);
  stack->elems[stack->top] = (sb_item_t){.kind = kind, .id = id};
  stack->top++;
}

int8_t sb_stack_find(sb_stack_t *stack, sb_kind_t kind, sb_id_t id) {
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (stack->elems[i].kind == kind & stack->elems[i].id == id)
      return i;
  }
  return -1;
}

// shadow map that associates a borrow ID to each pointer variable of the
// program The borrow ID is stored under the object ID of the memory location
// that contains the pointer variable.
shadow_map_t __sb_id_map;

void sb_id_map_init() { shadow_map_init(&__sb_id_map, sizeof(sb_id_t)); }

void sb_id_map_set_ptr(void **ptr_to_ptr, sb_id_t id) {
  *(sb_id_t *)shadow_map_get(&__sb_id_map, ptr_to_ptr) = id;
}

void sb_id_map_set_local(void *address_of_local, sb_id_t id) {
  *(sb_id_t *)shadow_map_get(&__sb_id_map, address_of_local) = id;
}

sb_id_t sb_id_map_get_ptr(void **ptr_to_ptr) {
  return *(sb_id_t *)shadow_map_get(&__sb_id_map, ptr_to_ptr);
}

sb_id_t sb_id_map_get_local(void *address_of_local) {
  return *(sb_id_t *)shadow_map_get(&__sb_id_map, address_of_local);
}

// Shadow memory that associates pointers with borrow stacks
shadow_map_t __sb_stack_map;

// Initialises a shadow map of stack pointers
void sb_stack_map_init() {
  shadow_map_init(&__sb_stack_map, sizeof(sb_stack_t *));
}

// Gets the borrow stack associated with the memory location pointed to by ptr.
sb_stack_t *sb_stack_get(void *ptr) {
  sb_stack_t **shadow_stack = shadow_map_get(&__sb_stack_map, ptr);
  if (!*shadow_stack) {
    *shadow_stack = sb_stack_create();
    (*shadow_stack)->top = 0;
  }
  return *shadow_stack;
}

// initialise ghost state for stacked borrows
#define SB_INIT(symbolic_size, max_stack_size)                                 \
  do {                                                                         \
    SB_SYMSIZE = symbolic_size;                                                \
    SB_MAX_STACK_SIZE = max_stack_size;                                        \
    sb_id_map_init();                                                          \
    sb_stack_map_init();                                                       \
  } while (0)

////// stacked borrows rules from the paper //////

// Initialises the borrow stack for a local object by creating the borrow stack
// in the map and pushing a SB_UNIQUE on that stack. The local variable
// owns itself and a direct write to the variable is treated like a write
// through a mutable ref.
#define NEW_LOCAL(local) sb_new_local(&local)
void sb_new_local(void *ptr) {
  sb_id_t fresh_id = sb_id_fresh();
  sb_id_map_set_local(ptr, fresh_id);
  sb_stack_push(sb_stack_get(ptr), SB_UNIQUE, fresh_id);
}

// Initialises the borrow stack for the dynamic object pointed to by
// the pointer variable pointed to by ptr. Dynamic objects are anonymous
// and can only be referred to throught the pointer variable that received
// the fresh pointer value. That pointer variable uniquely owns the object.
#define NEW_DYNAMIC(ptr) sb_new_dynamic(&ptr)
void sb_new_dynamic(void **ptr) {
  sb_id_t fresh_id = sb_id_fresh();
  sb_id_map_set_ptr(ptr, fresh_id);
  sb_stack_push(sb_stack_get(*ptr), SB_UNIQUE, fresh_id);
}

#define UNIQUE_FROM_LOCAL(new_ref, local)                                      \
  sb_new_mut_from_local(&new_ref, &local)

// Models the creation of a new mutable reference created from the address of a
// local variable.
void sb_new_mut_from_local(void **new_ref, void *local) {
  sb_id_t old_id = sb_id_map_get_local(local);
  sb_id_t new_id = sb_id_fresh();
  sb_id_map_set_ptr(new_ref, new_id);
  sb_stack_push(sb_stack_get(local), SB_UNIQUE, new_id);
}

#define UNIQUE_FROM_REF(new_ref, old_ref)                                      \
  sb_new_mut_from_ref(&new_ref, &old_ref)

// Models a new mutable reference created by borrowing an existing reference.
// let &mut y = x;
void sb_new_mut_from_ref(void **new_ref, void **old_ref) {
  sb_id_t old_id = sb_id_map_get_ptr(old_ref);
  sb_id_t new_id = sb_id_fresh();
  sb_id_map_set_ptr(new_ref, new_id);
  sb_stack_push(sb_stack_get(*old_ref), SB_UNIQUE, new_id);
}

// USE-1 Rule from the paper. Triggered when a memory location is updated
// through a reference. Checks that the Unique(id) of that reference is in the
// stack and pop anything above it.
// e.g.
// let local = 12; // new_local [Unique(0)]
// let &mut x = &mut local; // new_mut_ref_from_local [Unique(0), Unique(1)]
// let &mut y = x; // new_mut_ref_from_ref [Unique(0), Unique(1), Unique(2)]
// *x = ...; // use-1 [Unique(0), Unique(1)]
// *y = ...; // use-1 FAILURE

#define USE1_LOCAL(used)                                                       \
  do {                                                                         \
    bool result = sb_use1_local(&used);                                        \
    __CPROVER_assert(result, "USE1 " #used);                                   \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_use1_local(void *used) {
  sb_id_t used_id = sb_id_map_get_local(used);
  sb_stack_t *stack = sb_stack_get(used);
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (stack->elems[i].id == used_id && stack->elems[i].kind == SB_UNIQUE) {
      stack->top = i + 1;
      return true;
    }
  }
  return false;
}

#define USE1(used)                                                             \
  do {                                                                         \
    bool result = sb_use1(&used);                                              \
    __CPROVER_assert(result, "USE1 " #used);                                   \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_use1(void **used) {
  sb_id_t used_id = sb_id_map_get_ptr(used);
  sb_stack_t *stack = sb_stack_get(*used);
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (stack->elems[i].id == used_id && stack->elems[i].kind == SB_UNIQUE) {
      stack->top = i + 1;
      return true;
    }
  }
  return false;
}

#define SHARED_RW_FROM_LOCAL(new_raw, local)                                   \
  sb_new_raw_from_local(&new_raw, &local)

// New raw pointer from the address of a local variable.
void sb_new_raw_from_local(void **new_raw, void *local) {
  sb_id_map_set_ptr(new_raw, __sb_id_bottom);
  sb_stack_push(sb_stack_get(local), SB_SHARED_RW, __sb_id_bottom);
}

#define SHARED_RW_FROM_REF(new_raw, old_ref)                                   \
  sb_new_raw_from_ref(&new_raw, &old_ref)

// New raw pointer from a reference.
void sb_new_raw_from_ref(void **new_raw, void **old_ref) {
  sb_id_map_set_ptr(new_raw, __sb_id_bottom);
  sb_stack_push(sb_stack_get(*old_ref), SB_SHARED_RW, __sb_id_bottom);
}

#define TRANSMUTE_REF(new_ref, old_ref) sb_transmute_ref(&new_ref, &old_ref)

// Transmuting a ref to another ref copies the borrow id but does not modify
// the stack.
void sb_transmute_ref(void **new_ref, void **old_ref) {
  sb_id_map_set_ptr(new_ref, sb_id_map_get_ptr(old_ref));
}

// USE-2 Rule from the paper (replaces USE-1).
// When writing to a memory location check that a mutable ref or raw pointer
// borrow with same ID is in the stack and pop anything above it.

#define USE2_LOCAL(used)                                                       \
  do {                                                                         \
    bool result = sb_use2_local(&used);                                        \
    __CPROVER_assert(result, "USE2 " #used);                                   \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_use2_local(void *used) {
  sb_id_t used_id = sb_id_map_get_local(used);
  sb_kind_t kind = (used_id == __sb_id_bottom) ? SB_SHARED_RW : SB_UNIQUE;
  sb_stack_t *stack = sb_stack_get(used);
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (stack->elems[i].id == used_id && stack->elems[i].kind == kind) {
      stack->top = i + 1;
      return true;
    }
  }
  return false;
}

#define USE2(used)                                                             \
  do {                                                                         \
    bool result = sb_use2(&used);                                              \
    __CPROVER_assert(result, "USE2 " #used);                                   \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_use2(void **used) {
  sb_id_t used_id = sb_id_map_get_ptr(used);
  sb_kind_t kind = (used_id == __sb_id_bottom) ? SB_SHARED_RW : SB_UNIQUE;
  sb_stack_t *stack = sb_stack_get(*used);
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (stack->elems[i].id == used_id && stack->elems[i].kind == kind) {
      stack->top = i + 1;
      return true;
    }
  }
  return false;
}

#define SHARED_RO_FROM_LOCAL(new_ref, local)                                   \
  sb_new_shared_from_local(&new_ref, &local)

// New mutable reference created from the address of a stack variable.
void sb_new_shared_from_local(void **new_ref, void *local) {
  sb_id_t new_id = sb_id_fresh();
  sb_id_map_set_ptr(new_ref, new_id);
  sb_stack_push(sb_stack_get(local), SB_SHARED_RO, new_id);
}

#define SHARED_RO_FROM_REF(new_ref, old_ref)                                   \
  sb_new_shared_from_ref(&new_ref, &old_ref)

// New mutable reference created by copying an existing reference.
void sb_new_shared_from_ref(void **new_ref, void **old_ref) {
  sb_id_t new_id = sb_id_fresh();
  sb_id_map_set_ptr(new_ref, new_id);
  sb_stack_push(sb_stack_get(*old_ref), SB_SHARED_RO, new_id);
}

// READ-1 Rule from the paper. Check that the used borrow id in the stack
// and pop anything but Shared_RO above it.
// We go from the bottom of the stack to the top looking for the tag,
// when found we continue scanning until the first element that's not SHARED_RO
// and set this as top of stack.

#define READ1_LOCAL(used)                                                      \
  do {                                                                         \
    bool result = sb_read1_local(&used);                                       \
    __CPROVER_assert(result, "READ1 " #used);                                  \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_read1_local(void *used) {
  sb_id_t used_id = sb_id_map_get_local(used);
  sb_stack_t *stack = sb_stack_get(used);
  bool found = false;
  int8_t new_top = -1;
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (!found) {
      found = stack->elems[i].id == used_id;
      new_top = i;
    } else {
      if (stack->elems[i].kind == SB_SHARED_RO) {
        new_top = i;
      } else {
        break;
      }
    }
  }
  if (found)
    stack->top = new_top + 1;
  return found;
}

#define READ1(used)                                                            \
  do {                                                                         \
    bool result = sb_read1(&used);                                             \
    __CPROVER_assert(result, "READ1 " #used);                                  \
    if (!result)                                                               \
      __CPROVER_assume(false);                                                 \
  } while (0)

bool sb_read1(void **used) {
  sb_id_t used_id = sb_id_map_get_ptr(used);
  sb_stack_t *stack = sb_stack_get(*used);
  bool found = false;
  int8_t new_top = -1;
  for (int8_t i = 0; (i < SB_MAX_STACK_SIZE) && (i < stack->top); i++) {
    if (!found) {
      found = stack->elems[i].id == used_id;
      new_top = i;
    } else {
      if (stack->elems[i].kind == SB_SHARED_RO) {
        new_top = i;
      } else {
        break;
      }
    }
  }
  if (found)
    stack->top = new_top + 1;
  return found;
}

#endif
