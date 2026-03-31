int add(int x, int y)
{
    int z = x + y;
    return z;
}

int main()
{
    int a = 2;
    int b = 3;
    if (a < b)
    {
        a += 10;
    }
    return add(a, b);
}