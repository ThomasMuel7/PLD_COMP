int main() {
    int a;
    int b;
    a = 0xF; // 15
    b = 0xA; // 10
    return (a & b) | (a ^ b); // should compute 15&10=10 then XOR->? but just combine
}
