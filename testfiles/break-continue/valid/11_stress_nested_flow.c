int main() {
    int i;
    int j;
    int s;
    i = 0;
    s = 0;
    while (i < 5) {
        j = 0;
        while (j < 5) {
            j += 1;
            if (j == 2) {
                continue;
            }
            if (j == 5) {
                break;
            }
            s += j;
        }
        if (i == 3) {
            break;
        }
        i += 1;
    }
    return s;
}
