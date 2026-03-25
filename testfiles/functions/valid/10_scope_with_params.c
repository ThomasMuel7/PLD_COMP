int f(int x) {
    int y;
    y = x + 1;
    {
        int x;
        x = 10;
        y = y + x;
    }
    return y;
}

int main() {
    return f(2);
}
