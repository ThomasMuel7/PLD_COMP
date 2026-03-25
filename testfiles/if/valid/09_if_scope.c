int main() {
  int x;
  x = 1;
  if (1) {
    int y;
    y = 10;
    x = y + x;
  }
  return x;
}
