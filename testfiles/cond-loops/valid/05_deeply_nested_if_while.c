int main() {
  int x;
  int y;
  x = 0;
  y = 0;
  while (x < 2) {
    y = 0;
    while (y < 2) {
      if (x == 0) {
        if (y == 0) {
          return 11;
        }
      } else {
        if (y == 0) {
          return 21;
        }
      }
      y = y + 1;
    }
    x = x + 1;
  }
  return 0;
}
