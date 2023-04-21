#include "stacked_borrows.h"

/// shared references
int main() {
  SB_INIT(true, 8);

  // let mut local = 6;
  int local = 6;
  NEW_LOCAL(local);

  // let x = &local;
  USE2_LOCAL(local);
  int *x = &local;
  SHARED_RO_FROM_LOCAL(x, local);

  // fn example2(x: &i32, f: impl FnOnce(&i32)) -> i32 {
  //   retag x;
  //   let val = *x / 3;
  //   f(x);
  //   return *x / 3;
  // }
  //
  // let result = example2(x, |inner_x| {
  //   retag inner_x;
  //   let raw_pointer: *mut i32 = unsafe {mem::transmute(inner_x)};
  //   unsafe { *raw_pointer = 15 };
  // });

  // inline example2
  int result;
  {
    READ1(x);
    int *example2_x = x;
    SHARED_RO_FROM_REF(example2_x, x); // retag example2::x

    READ1(example2_x);
    int val = *example2_x / 3;
    // inline closure
    {
      READ1(example2_x);
      int *inner_x = example2_x;
      SHARED_RO_FROM_REF(inner_x, example2_x); // retag closure::inner_x
      // let raw_pointer: *mut i32 = unsafe {mem::transmute(inner_x)};
      int *raw_pointer = inner_x;
      TRANSMUTE_REF(raw_pointer, inner_x);
      // unsafe { *raw_pointer = 15 };
      USE2(raw_pointer); // should fail
      *raw_pointer = 15;
    }
    result = *example2_x / 3;
  }
  assert(result = 2); // pass because we assume(false) on UB
  return 0;
}