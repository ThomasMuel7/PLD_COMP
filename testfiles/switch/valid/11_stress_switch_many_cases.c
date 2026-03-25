int main() {
    int x;
    int s;
    x = 4;
    s = 0;
    switch (x) {
        case 1: s = 10; break;
        case 2: s = 20; break;
        case 3: s = 30; break;
        case 4: s = 40;
        case 5: s += 2; break;
        default: s = 99;
    }
    return s;
}
