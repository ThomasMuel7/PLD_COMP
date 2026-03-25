int main() {
    int x;
    int y;
    x = 1;
    y = 0;
    switch (x) {
        case 1:
            y += 3;
        case 2:
            y += 4;
            break;
        default:
            y += 100;
    }
    return y;
}
