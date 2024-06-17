#ifdef DEMONIC
#include "stacked_borrows_demonic.h"
#else
#include "stacked_borrows.h"
#endif

int main() {
  SB_INIT(true, 16);

  // let mut local = 5;
  int32_t local = 5;
  NEW_LOCAL(local);

  // let raw_pointer1 = &mut local as *mut i32;
  USE2_LOCAL(local);
  int32_t *raw_pointer1 = &local;
  SHARED_RW_FROM_LOCAL(raw_pointer1, local);

  // let raw_pointer2 = &mut local as *mut i32;
  USE2_LOCAL(local);
  int32_t *raw_pointer2 = &local;
  SHARED_RW_FROM_LOCAL(raw_pointer2, local);

  // let result = unsafe{ example1(&mut *raw_pointer, &mut *raw_pointer) }
  // fn example1(x: *mut i32, y: *mut i32) -> i32
  int32_t __example1_return_value;
  {
    // argument passing
    // *mut x receives &mut *raw_pointer
    USE2(raw_pointer1);
    int32_t *x = raw_pointer1;
    SHARED_RW_FROM_REF(x, raw_pointer1);

    // &mut y receives &mut *raw_pointer
    USE2(raw_pointer2);
    int32_t *y = raw_pointer2;
    SHARED_RW_FROM_REF(y, raw_pointer2);

    // function body
    USE2(x);
    *x = 42;

    USE2(y);
    *y = 13;

    READ1(x);
    __example1_return_value = *x;
  }
  return 0;
}