int main() {
    int a[4];
    int *p;
    a[0] = 2;
    a[1] = 4;
    a[2] = 6;
    a[3] = 8;
    p = a;
    return *(p + 3);
}
