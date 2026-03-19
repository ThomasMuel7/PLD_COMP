int main() {
  int i;
  i = 0;
  if (1) {
    while (i < 3) {
      if (i < 2) {
        i = i + 1;
      } else {
        return i;
      }
    }
  }
  return 0;
}
