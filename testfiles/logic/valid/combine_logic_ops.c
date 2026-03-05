int main() {
    int a, b, c, d;
    a = 1; // 0001
    b = 3; // 0011
    c = 6; // 0110
    d = 15; // 1111
    return (a & b) | (c ^ d);
}