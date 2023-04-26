#include "stacked_borrows.h"
#include <stdbool.h>

bool nondet_bool();

int main() {
  SB_INIT(true, 16);

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

  if (nondet_bool()) {
    USE1(x);
    *x += 1;
  }

  if (nondet_bool()) {
    USE1(y);
    *y = 2;
  }

  return 0;
}
