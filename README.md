Prototype of a Stacked Borrows instrumentation pass


# Stacked Borrows overview

The Stacked Borrows paper defines a (virtual) operational model for the Rust
borrow checker. A shadow stack of borrows is used to keep track of which
references or pointers are pointing to a memory location.
Stack updates and checks performed when reading/writing to a location through a
given borrow allows to define and detect the occurrence of classes of undefined
behaviour.

# High-level overview of stacked borrows

Each reference gets assigned a unique _tag_, pointers get a default $\bot$ tag.
This allows to distinguish references from references, and pointers from references,
but this purposedly does not let us distinguish between different pointers.

Each memory location gets mapped to a _borrow stack_.

Any time a new reference or a new pointer to the location is created in the program,
an pair (access type, pointer tag) is pushed is pushed at the top
of the stack. The access type reflects the type of reference
(exclusive read-write for mutable borrows, shared-read-only for immutable borrows,
shared-read-write for raw pointers).

At any time, by looking a the stack we know which pointers are
pointing to the location and the order in which they were created.

Reading from a location through a pointer with tag `t` checks that the
tag `t` is found in the stack, and fails otherwise.

Writing to a location through a pointer with tag `t` checks that the
tag `t` is found in the stack and has the proper access type,
and fails otherwise. The stack then gets popped to put the most recently used
tag at the top of the stack.

This discipline allows to detect interleaved ABAB access patterns :

```rust
let location: T = .. ;
// pointerID(&location) = 0;
// stack(&location) = [Unique(0)];

// reading &location: check that Unique(0) is in the stack
let a: &mut T = &mut location;
// pointerID(a) = 1;
// stack(&location) = [Unique(0), Unique(1)];

// reading a: check that Unique(1) is in the stack
let b: &mut T = b;
// pointerID(b) = 2;
// stack(&location) = [Unique(0), Unique(1), Unique(2)];

// writing through a : check that Unique(1) is in the stack and pop Unique(2)
*a = .. ;
// stack(&location) = [Unique(0), Unique(1)];

// writing through b : check that Unique(2) is in the stack, FAIL
*b = .. ;   // B
```

Popping the stack after an update through `a` models that reference `b` borrowed
from `a` dies when `a` gets used again (it is an error to access `location`
through `b` once `a` has been used again).


# modelling shadow memory in CBMC

We use the shadow memory concept to 

# limitations of the prototype

The stacks are statically bounded to some size
(required so that the symbolic execution of stack operation in CBMC
always terminates).

