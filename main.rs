struct S {
  a: i32,
  b: i32,
}
fn main() {
  let mut s: S = S {a : 12, b:12};
  let a = &mut s.a;
  let b = &mut s.b;
  *a +=1;
  *b +=1;
}