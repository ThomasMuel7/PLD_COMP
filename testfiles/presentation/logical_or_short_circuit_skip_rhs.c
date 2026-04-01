int main() {
    int x;
    x = 0;
    if (1 || (x = 5)) {
        return x + 100;
    }
    return x;
}
