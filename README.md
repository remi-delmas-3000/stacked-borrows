# Stacked Borrows instrumentation for formal verification on top of CBMC

## TL;DR

The Stacked Borrows paper defines an operational model for the Rust borrow checker.

Each memory location is virtually equipped with a stack that keeps track of the various borrows (`&T`, `&mut T` , `*mut T`, `*const T)` currently active for that location.  The dynamic lifetime of a borrow corresponds to the interval for which it stays in the stack.

As the program runs, new borrows are pushed on the top of the stack. References get tagged with unique identifiers and raw pointers get tagged with a default ⊥ tag. Read and write accesses through references or pointers check that the stack contains a borrow with the same tag and appropriate read/write permissions to decide if the access is legal. In addition, write accesses pop the top of the stack in order to invalidate borrows that are above the one used to perform the access.

The LIFO discipline enforced by the stack represents the fact that, in a well behaved program, the lifetimes of borrows should be nested.

Violating the stacked borrows disciplined is by definition Undefined Behaviour.

In this experiment we developped:

* a naive C implementation of the borrow stack data type and functions defined in the reference paper,
* a simple shadow memory model that lets us associate a stack with arbitrary memory locations,
* manually instrumented C programs that model the examples from the paper. The instrumentation consists in
    * inserting calls to the instrumentation function to initialise the shadow memory,
    * push elements in the borrow stack when new borrows are created
    * calling stack update functions and inserting assertion checks for each read/write access.

# High-level introduction to stacked borrows

Each memory location gets mapped to a *borrow stack*. The borrow stack contains elements representing the different *borrows or pointers* pointing to the memory location. A borrow is modelled as a follows:

```
Borrow ::= Unique(Tag) | SharedRO(Tag) | SharedRW(⊥)
```

* `Unique(Tag)` is used for `&mut T`, 
* `SharedRO(Tag)` is used for `&T`, 
* `SharedRW(⊥`) is used for  `*mut T`. 

The `Tag` uniquely identifies the borrow for  `Unique` and `SharedRO` borrows. Tagging  values in this way allows us to distinguish references from references, pointers from references, but purposely does not let us distinguish between pointers (more on that below). 

Any time a new reference or pointer to a location is created, a borrow is pushed on the stack and the reference gets tagged with its unique tag, or ⊥ if it is a raw pointer.

Every read access to a location through a reference with tag `t` checks that a borrow `Unique(t)` or `SharedRW(t)` or `SharedRO(t)`is found in the stack.

Every write access to a location through a pointer with tag `t` checks that a borrow `Unique(t)` or `SharedRW(t)` is found in the stack, and pops the top elements of the stack to put the borrow at the top of the stack. Popping the stack makes it impossible to use one of the popped borrows to access the location again. This discipline allows to detect and reject interleaved ABAB access patterns:

```rust
let location: T = .. ;
// Tag(&location) = 0;
// Stack(&location) = [Unique(0)];
// reading &location: check that Unique(0) is in the stack

let a: &mut T = &mut location;
// Tag(a) = 1;
// Stack(&location) = [Unique(0), Unique(1)];
// reading a: check that Unique(1) is in the stack

let b: &mut T = b;
// Tag(b) = 2;
// Stack(&location) = [Unique(0), Unique(1), Unique(2)];

// writing through a with Tag 1:
// - check that Unique(1) is in the stack
// - pop Unique(2)
*a = .. ;
// Stack(&location) = [Unique(0), Unique(1)];

// writing through b with Tag 2:
// - check that Unique(2) is in the stack -> FAILURE
*b = .. ;
```

Popping the stack after an update through `a` models the end of the lifetime of the reference `b` borrowed from `a`. One way to view it: it is illegal to keep using a mutable reference once the memory location has been updated using a reference *underneath it in the stack.* The actual rules of stacked borrows are more complex that this and use operations like *retagging* and *protectors* to handle more subtle cases and avoid rejecting too many programs while preserving the  `mutability XOR aliasing` core guarantees.

# Instrumentation using shadow maps

In the stacked borrows model, a borrow stack can potentially be associated with every memory location, down to the byte level. We need to define a generic mechanism to map shadow state to any byte of data manipulated by the user program.

## Shadow memory model overview

* The shadow memory model associates a shadow object to any user object.
* A shadow object provides k shadow bytes for each byte of its source object.
* A shadow memory map is a map from object IDs to pointers to lazily allocated shadow objects.

```c
shadow_map_t __shadow_map;
```

Given an pointer `ptr` pointing to some byte `b` in memory, a pointer to the `k` shadow bytes for `b` is obtained by changing the base address of `ptr` and scaling its offset by a factor `k`:

```c
void *shadow_ptr =
  __shadow_map[__CPROVER_POINTER_OBJECT(ptr)] + k*__CPROVER_POINTER_OFFSET(ptr);
```

It is possible to instantiate several different shadow maps with different `k` values in the model.

## Implementation details

- A shadow memory map contains the shadow bytes per byte factor and an array of pointers to shadow objects.
- The shadow map is initialised to the maximum possible number of objects that symex can handle.
- The only available operation is to rebase a user pointer to a shadow pointer.

```c
typedef struct {
  /** nof shadow bytes per byte */
  size_t k;
  /** pointers to shadow objects */
  void **ptrs;
} shadow_map_t;

extern size_t __CPROVER_max_malloc_size;
int __builtin_clzll(unsigned long long);

/** computes 2^OBJECT_BITS */
#define __nof_objects                                                          \
  ((size_t)(1ULL << __builtin_clzll(__CPROVER_max_malloc_size)))

/** Initialises the given shadow memory map. */
void shadow_map_init(shadow_map_t *smap, size_t k) {
  *smap = (shadow_map_t){
      .k = k, .ptrs = __CPROVER_allocate(__nof_objects * sizeof(void *), 1)};
}

/** Translates the pointer to its shadow pointer. */
void *shadow_map_get(shadow_map_t *smap, void *ptr) {
  size_t id = __CPROVER_POINTER_OBJECT(ptr);
  void *sptr = smap->ptrs[id];
  if (!sptr) {
    sptr = __CPROVER_allocate(smap->k * __CPROVER_OBJECT_SIZE(ptr), 1);
    smap->ptrs[id] = sptr;
  }
  return sptr + smap->k * __CPROVER_POINTER_OFFSET(ptr);
}
```

# Stacked borrow instrumentation & experiments

The repository https://github.com/remi-delmas-3000/stacked-borrows contains the model.

It implements the first basic rules defined in the stacked borrows paper until section 4.

Our model does not yet include the more advanced stack protection and unique borrows disabling rules discussed in sections 4 and 5 of the paper.

Using the SAT or SMT back end of CBMC, we are able to analyse the examples, and either find counter examples that violate the stacked borrow rules, or prove that programs are correct with respect to stacked borrow rules.

## Conclusion

These experiments show that encoding the stacked borrows rules in a form that is understandable by CBMC is feasible at least in theory.

Since the instrumentation requires knowing the type of references to perform the instrumentation, it would have to be performed in Kani where type information is available. The instrumentation could work by inserting calls to the C functions implementing the stacked borrows rules provided as a library with a public interface.
