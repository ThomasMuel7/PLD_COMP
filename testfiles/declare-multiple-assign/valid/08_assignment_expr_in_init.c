int main()
{
  int a, b = 2, c = (a = 3) + b;
  return a + c;
}
