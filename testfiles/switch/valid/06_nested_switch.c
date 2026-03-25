int main() {
    int a;
    int b;
    a = 2;
    b = 1;
    switch (a) {
        case 2:
            switch (b) {
                case 1:
                    return 21;
                default:
                    return 22;
            }
        default:
            return 0;
    }
}
