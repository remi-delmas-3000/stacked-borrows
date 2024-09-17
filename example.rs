

fn main() {

  let mut x: i32 = 0;
  // RootTag(&x) = 1
  // Tag(&x) = Unique(1)

  let mut y: i32 = 0;
  // RootTag(&y) = 2
  // Tag(&y) = Unique(2)

  let px = &mut x;
  // RootTag(&px) = 3
  // Tag(&x) = Unique(1)
  // Tag(px) = Unique(4)

  let px2 = &x;
  // RootTag(&px2) = 5
  // Tag(&x) = Unique(1)
  // Tag(&y) = Unique(2)
  // Tag(px) = Unique(4)
  // Tag(px2) = Unique(5)

  let felipe = &px;
  // RootTag(&felipe) = 6
  // set_root_tag(&felipe, 6)
  // Tag(felipe) = Unique(3)
  // set_tag(&felipe, 3);

  let felipe2 = felipe;

  *px // UB



}