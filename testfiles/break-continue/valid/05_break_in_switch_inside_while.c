int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 4) {
        switch (i) {
            case 2:
                break;
            default:
                s += 1;
                break;
        }
        i += 1;
    }
    return s;
}
