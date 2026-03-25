int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 6) {
        i += 1;
        if ((i % 2) == 0) {
            continue;
        }
        s += i;
    }
    return s;
}
