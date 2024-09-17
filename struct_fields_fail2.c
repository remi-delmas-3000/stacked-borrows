#include "stacked_borrows_demonic_size.h"

typedef struct
{
  int a;
  int b;
} my_struct_t;

int main()
{
  SB_INIT(true, 16);

  my_struct_t s;

  // for structs we initialise one stack for the whole struct
  NEW_LOCAL(s);

  // now we borrow each field separately

  // let a = &mut s.a;
  USE1_LOCAL(s.a);
  int *a = &s.a;
  UNIQUE_FROM_LOCAL(a, s.a);

  // let b = &mut s.b;
  USE1_LOCAL(s.b);
  int *b = &s.b;
  UNIQUE_FROM_LOCAL(b, s.b);

  if (nondet_bool())
  {
    // *b += 1;
    USE1(b);
    *b += 1;
  }

  if (nondet_bool())
  {
    // *a += 1;
    USE1(a);
    *a += 1;
  }

}
