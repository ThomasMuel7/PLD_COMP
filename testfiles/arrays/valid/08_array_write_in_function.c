int write_second(int *p, int v) {
    p[1] = v;
    return p[1];
}

int main() {
    int a[2];
    a[0] = 5;
    a[1] = 0;
    return write_second(a, 17);
}
