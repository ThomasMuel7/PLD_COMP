int main()
{
  int x;
  x = 5;
  {
    int y;
    y = 10;
    {
      int z;
      z = x + y;
      return z;
    }
  }
}
