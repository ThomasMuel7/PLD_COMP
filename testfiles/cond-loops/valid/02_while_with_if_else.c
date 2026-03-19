int main() {
  int i;
  int result;
  i = 0;
  result = 0;
  while (i < 3) {
    if (i == 1) {
      result = result + 10;
    } else {
      result = result + 1;
    }
    i = i + 1;
  }
  return result;
}
