int main() {
  int a;
  int b;
  int c;
  a = 2;
  b = 3;
  c = 5;
  while (a > 0) {
    while (b > 0) {
      while (c > 0) {
        c = c - 1;
      }
      b = b - 1;
      c = 5;
    }
    a = a - 1;
    b = 3;
  }
  return 1;
}
