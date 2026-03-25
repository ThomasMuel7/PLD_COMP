int main() {
    int i;
    int j;
    int x;
    i = 0;
    x = 0;
    while (i < 3) {
        j = 0;
        while (j < 3) {
            if (j == 1) {
                break;
            }
            x += 2;
            j += 1;
        }
        i += 1;
    }
    return x;
}
