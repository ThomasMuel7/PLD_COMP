int main() {
    int a, b, c;
    a = 5; // 0101
    b = 3; // 0011
    c = 1; // 0001
    return a ^ b ^ c; // (5 ^ 3) ^ 1 = 6 ^ 1 = 7
}