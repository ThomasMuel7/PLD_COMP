void bump(int x) {
    int y;
    y = x + 2;
    return;
}

int id(int x) {
    return x;
}

int main() {
    bump(4);
    return id(9);
}
