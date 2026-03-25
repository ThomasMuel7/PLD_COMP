int main() {
    int i;
    int j;
    int s;
    i = 0;
    s = 0;
    while (i < 3) {
        j = 0;
        while (j < 4) {
            j += 1;
            if (j == 2) {
                continue;
            }
            s += j;
        }
        i += 1;
    }
    return s;
}
