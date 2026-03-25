int main()
{
  int a;
  a = 1;
  {
    int b;
    b = a + 1;
  }
  {
    int c;
    c = a + 2;
  }
  {
    int d;
    d = a + 3;
    return d;
  }
}
