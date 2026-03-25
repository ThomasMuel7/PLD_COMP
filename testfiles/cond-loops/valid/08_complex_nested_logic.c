int main() {
  int a;
  int b;
  int result;
  a = 3;
  b = 2;
  result = 0;
  while (a > 0) {
    b = 2;
    while (b > 0) {
      if (a > b) {
        result = result + 1;
      }
      b = b - 1;
    }
    a = a - 1;
  }
  return result;
}
