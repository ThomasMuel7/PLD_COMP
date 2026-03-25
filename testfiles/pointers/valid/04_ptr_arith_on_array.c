int main() {
    int t[3];
    int *p;
    t[0] = 3;
    t[1] = 7;
    t[2] = 12;
    p = &t;
    return *(p + 1);
}
