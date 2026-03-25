int main() {
    int t[4];
    int *p;
    int i;
    int s;
    t[0] = 1;
    t[1] = 2;
    t[2] = 3;
    t[3] = 4;
    p = &t;
    i = 0;
    s = 0;
    while (i < 4) {
        s += *(p + i);
        i += 1;
    }
    return s;
}
