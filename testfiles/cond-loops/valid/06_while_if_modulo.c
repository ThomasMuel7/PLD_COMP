int main() {
  int counter;
  int value;
  counter = 1;
  value = 0;
  while (counter <= 5) {
    if (counter % 2 == 0) {
      value = value + counter;
    } else {
      value = value + 1;
    }
    counter = counter + 1;
  }
  return value;
}
