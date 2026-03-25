int main() {
    int a[6];
    int i;
    int s;
    i = 0;
    while (i < 6) {
        a[i] = i;
        i += 1;
    }
    s = 0;
    i = 0;
    while (i < 6) {
        s += a[i];
        a[i] += 1;
        s += a[i];
        i += 1;
    }
    return s;
}