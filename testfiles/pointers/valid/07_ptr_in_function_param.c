int set99(int *p) {
    *p = 99;
    return *p;
}

int main() {
    int a;
    a = 0;
    return set99(&a);
}
