#include "stacked_borrows.h"

int main() {
  SB_INIT(true, 8);

  // let mut local = 42;
  int local = 42;
  NEW_LOCAL(local);

  // let x = &mut local;
  int *x = &local;
  USE1_LOCAL(local);
  UNIQUE_FROM_LOCAL(x, local);

  // let y = &mut *x;
  int *y = x;
  USE1(x);
  UNIQUE_FROM_REF(y, x);

  USE1(y);
  *y = 2;

  USE1(x);
  *x += 1;

  return 0;
}
