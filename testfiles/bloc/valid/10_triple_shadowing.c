int main()
{
  {
    int x;
    x = 1;
    {
      int x;
      x = 2;
      {
        int x;
        x = 3;
        return x;
      }
    }
  }
}
