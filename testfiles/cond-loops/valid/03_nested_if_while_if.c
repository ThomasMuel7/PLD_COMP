int main() {
  int a;
  int b;
  int c;
  a = 1;
  b = 1;
  c = 0;
  while (a <= 2) {
    while (b <= 2) {
      if (a == 1 && b == 1) {
        c = 11;
      } else if (a == 2 && b == 2) {
        c = 22;
      } else {
        c = 0;
      }
      b = b + 1;
    }
    a = a + 1;
    b = 1;
  }
  return c;
}
