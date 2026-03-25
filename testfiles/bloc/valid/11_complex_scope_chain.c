int main()
{
  int x;
  x = 5;
  {
    int y;
    y = x + 10;
    {
      int z;
      z = y * 2;
      x = z;
    }
  }
  return x;
}
