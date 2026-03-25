int main() {
    int a;
    int b;
    int *p;
    a = 5;
    b = 11;
    p = &a;
    p = &b;
    return *p;
}
