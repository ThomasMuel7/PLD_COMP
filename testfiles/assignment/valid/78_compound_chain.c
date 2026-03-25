int main()
{
  int a, b;
  a = 3;
  b = 2;
  return (a += (b += 4)); // b = 6, a = 9 -> should return 9
}
