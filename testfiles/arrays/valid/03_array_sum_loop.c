int main() {
    int a[4];
    int i;
    int s;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    i = 0;
    s = 0;
    while (i < 4) {
        s += a[i];
        i += 1;
    }
    return s;
}
