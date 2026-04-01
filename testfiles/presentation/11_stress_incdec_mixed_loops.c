int main() {
    int i;
    int j;
    int s;
    i = 0;
    s = 0;
    while (i++ < 4) {
        j = 3;
        while (j--) {
            s += ++i;
            if (i > 8) {
                break;
            }
        }
    }
    return s;
}
