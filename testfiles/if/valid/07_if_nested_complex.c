int main() {
  int a;
  int b;
  int c;
  a = 10;
  b = 5;
  c = 3;
  if (a > b)
    if (b > c)
      return 42;
    else
      return 20;
  else
    if (a > c)
      return 15;
    else
      return 1;
}
