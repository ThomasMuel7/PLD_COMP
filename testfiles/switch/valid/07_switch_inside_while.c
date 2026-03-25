int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 4) {
        switch (i) {
            case 0:
            case 1:
                s += 1;
                break;
            default:
                s += 2;
                break;
        }
        i += 1;
    }
    return s;
}
