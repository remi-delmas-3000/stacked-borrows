#include "stacked_borrows.h"

int main() {
  SB_INIT(true, 8);

  // let mut local = 42;
  int local = 42;
  NEW_LOCAL(local);

  // let x = &mut local;
  USE1_LOCAL(local);
  int *x = &local;
  UNIQUE_FROM_LOCAL(x, local);

  // let y = &mut *x;
  USE1(x);
  int *y = x;
  UNIQUE_FROM_REF(y, x);

  USE1(x);
  *x += 1;

  USE1(y);
  *y = 2;

  return 0;
}
