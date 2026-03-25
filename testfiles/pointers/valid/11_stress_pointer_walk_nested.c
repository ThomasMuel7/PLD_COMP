int main() {
    int t[6];
    int *p;
    int i;
    int j;
    int s;
    t[0] = 1; t[1] = 2; t[2] = 3; t[3] = 4; t[4] = 5; t[5] = 6;
    p = t;
    i = 0;
    s = 0;
    while (i < 3) {
        j = 0;
        while (j < 2) {
            s += *(p + (i * 2 + j));
            j += 1;
        }
        i += 1;
    }
    return s;
}
