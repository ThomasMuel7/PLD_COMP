int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 6) {
        switch (i) {
            case 1:
            case 3:
                i += 1;
                continue;
            default:
                break;
        }
        s += i;
        i += 1;
    }
    return s;
}
