int add_ptr(int *p, int *q) {
    return *p + *q;
}

int main() {
    int a;
    int b;
    a = 10;
    b = 32;
    return add_ptr(&a, &b);
}
