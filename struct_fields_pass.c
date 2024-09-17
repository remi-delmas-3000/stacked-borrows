#include "stacked_borrows_demonic_size.h"

typedef struct {
  int a;
  int b;
} my_struct_t;

int main() {
    SB_INIT(true, 16);

  my_struct_t my_struct;

  // for structs we initialise one stack for the whole struct
  NEW_LOCAL(my_struct);

  // now we borrow each field separately

  // create an ABBA on field a
  // let a1 = &mut my_struct.a;
  USE1_LOCAL(my_struct.a);
  int *a1 = &my_struct.a;
  UNIQUE_FROM_LOCAL(a1, my_struct.a);

  // let a2 = &mut *a1;
  USE1(a1);
  int *a2 = a1;
  UNIQUE_FROM_REF(a2, a1);

  if (nondet_bool()) {
    USE1(a2);
    *a2 = 2;
  }

  if (nondet_bool()) {
    USE1(a1);
    *a1 += 1;
  }

  // create an ABBA on field b
  // let b1 = &mut my_struct.b;
  USE1_LOCAL(my_struct.b);
  int *b1 = &my_struct.b;
  UNIQUE_FROM_LOCAL(b1, my_struct.b);

  // let b2 = &mut *b1;
  USE1(b1);
  int *b2 = b1;
  UNIQUE_FROM_REF(b2, b1);

  if (nondet_bool()) {
    USE1(b2);
    *b2 = 2;
  }

  if (nondet_bool()) {
    USE1(b1);
    *b1 += 1;
  }
}