int main() {
    int a;
    int *p;
    a = 4;
    p = &a;
    switch (p) {
        case 0:
            return 0;
        default:
            return 1;
    }
}
