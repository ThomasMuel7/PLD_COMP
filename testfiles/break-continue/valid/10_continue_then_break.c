int main() {
    int i;
    int s;
    i = 0;
    s = 0;
    while (i < 10) {
        i += 1;
        if (i < 3) {
            continue;
        }
        if (i == 6) {
            break;
        }
        s += i;
    }
    return s;
}
