int main() {
    int x;
    x = 1;
    switch (x) {
        default:
            x = 2;
            break;
        case 1:
            x = 3;
            break;
        default:
            x = 4;
            break;
    }
    return x;
}
