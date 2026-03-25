int main() {
    int x;
    int y;
    x = 3;
    y = 0;
    switch (x) {
        case 1:
            y = 10;
            break;
        default:
            y = 20;
            break;
        case 3:
            y = 30;
            break;
    }
    return y;
}
