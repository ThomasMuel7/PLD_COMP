int main()
{
  int a;
  a = 1;
  {
    int b;
    b = 2;
    {
      int c;
      c = a + b;
      return c;
    }
  }
}
