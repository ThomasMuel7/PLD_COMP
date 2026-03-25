int main() {
    int a;
    int *p;
    int *q;
    a = 8;
    p = &a;
    q = &a;
    if (p == q) {
        return 1;
    }
    return 0;
}
