int main() { auto p = new int(7); int value = *p; delete p; return value == 7 ? 0 : 1; }
