#include "stacked_borrows.h"

int main() {
  SB_INIT(true, 16);

  // let mut local = 5;
  int32_t local = 5;
  NEW_LOCAL(local);

  // let raw_pointer = &mut local as *mut i32;
  USE2_LOCAL(local);
  int32_t *raw_pointer = &local;
  SHARED_RW_FROM_LOCAL(raw_pointer, local);

  // fn example1(x: &mut i32, y: &mut i32) -> i32 { .. };
  // let result = unsafe{ example1(&mut *raw_pointer, &mut *raw_pointer) };
  int32_t __example1_return_value;
  {
    // argument passing
    // &mut x receives &mut *raw_pointer
    USE2(raw_pointer);
    int32_t *x = raw_pointer;
    UNIQUE_FROM_REF(x, raw_pointer); // this is a retag

    // &mut y receives &mut *raw_pointer
    USE2(raw_pointer); // pops x from the stack
    int32_t *y = raw_pointer;
    UNIQUE_FROM_REF(y, raw_pointer); // this is a retag

    // function body
    USE2(x); // fail
    *x = 42;

    USE2(y);
    *y = 13;

    READ1(x);
    __example1_return_value = *x;
  }
  return 0;
}