int first(int *p) {
    return p[0];
}

int main() {
    int a[3];
    a[0] = 33;
    a[1] = 9;
    a[2] = 1;
    return first(a);
}
