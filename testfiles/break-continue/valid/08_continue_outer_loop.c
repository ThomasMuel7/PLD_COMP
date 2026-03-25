int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 5) {
        i += 1;
        if (i < 3) {
            continue;
        }
        s += i;
    }
    return s;
}
