int main() {
    int i;
    int j;
    int c;
    i = 0;
    c = 0;
    while (i < 3) {
        j = 0;
        while (j < 5) {
            c += 1;
            break;
        }
        i += 1;
    }
    return c;
}
