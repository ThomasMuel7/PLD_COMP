int main() {
    int a, b, c, d, e;
    a = 1;
    b = 0;
    c = 1;
    d = 0;
    e = 1;
    return a & b & c | d ^ e; /* ((1 & 0 & 1) | 0) ^ 1 = (0 | 0) ^ 1 = 0 ^ 1 = 1 */
}