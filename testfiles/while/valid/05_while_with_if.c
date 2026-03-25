int main() {
  int x;
  int y;
  int z;
  x = 10;
  y = 0;
  z = 0;
  while (x > 0) {
    if (x % 2 == 0) {
      z = z + x;
    }
    x = x - 1;
  }
  return z;
}
