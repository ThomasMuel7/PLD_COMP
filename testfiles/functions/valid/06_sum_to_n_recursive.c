int sum_to_n(int n) {
    if (n <= 0) {
        return 0;
    }
    return n + sum_to_n(n - 1);
}

int main() {
    return sum_to_n(6);
}
