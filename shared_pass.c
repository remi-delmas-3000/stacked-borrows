#ifdef DEMONIC
#include "stacked_borrows_demonic.h"
#else
#include "stacked_borrows.h"
#endif

/// shared references
int main() {
  SB_INIT(true, 16);

  // let mut local = 42;
  int local = 42;
  NEW_LOCAL(local);

  // let x = &mut local;
  USE2_LOCAL(local);
  int *x = &local;
  UNIQUE_FROM_LOCAL(x, local);

  // let shared1 = &*x;
  READ1(x);
  int *shared1 = x;
  SHARED_RO_FROM_REF(shared1, x);

  // let shared2 = &*x;
  READ1(x);
  int *shared2 = x;
  SHARED_RO_FROM_REF(shared2, x);

  // let val1 = *x;
  READ1(x);
  int val1 = *x;

  // let val2 = *shared1;
  READ1(shared1);
  int val2 = *shared1;

  // let val3 = *shared2;
  READ1(shared2);
  int val3 = *shared2;

  // let val4 = *shared1;
  READ1(shared1);
  int val4 = *shared1;

  // *x += 17;
  USE2(x);
  *x += 17;

  return 0;
}