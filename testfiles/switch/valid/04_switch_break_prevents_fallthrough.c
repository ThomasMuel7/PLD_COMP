int main() {
    int x;
    int y;
    x = 1;
    y = 0;
    switch (x) {
        case 1:
            y += 5;
            break;
        case 2:
            y += 7;
            break;
        default:
            y += 9;
    }
    return y;
}
