#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ---------- Cac ham kiem tra dieu kien ----------
static void require(bool cond, const string& msg) {
    if (!cond) throw logic_error("VI PHAM: " + msg);
}

// Doan a[l..r] (bao gom hai dau) co tang dan khong giam hay khong
static bool isSortedRange(const vector<int>& a, int l, int r) {
    for (int k = l; k < r; ++k)
        if (a[k] > a[k + 1]) return false;
    return true;
}

// max cua a[0..j]
static int maxPrefix(const vector<int>& a, int j) {
    int m = a[0];
    for (int k = 1; k <= j; ++k) m = max(m, a[k]);
    return m;
}

// Hai mang co cung tap phan tu (hoan vi cua nhau) khong
static bool isPermutation(const vector<int>& a, const vector<int>& b) {
    if (a.size() != b.size()) return false;
    map<int, int> cnt;
    for (int x : a) ++cnt[x];
    for (int x : b)
        if (--cnt[x] < 0) return false;
    return true;
}

void bubbleSortChecked(vector<int>& a) {
    const int n = static_cast<int>(a.size());
    const vector<int> original = a;  // de kiem tra "hoan vi"

    if (n <= 1) return;  // mang rong / 1 phan tu da sap xep

    for (int i = 0; i < n - 1; ++i) {
        require(isSortedRange(a, n - i, n - 1), "N1 - duoi mang khong tang dan");
        if (i > 0)
            require(maxPrefix(a, n - i - 1) <= a[n - i], "N2 - phan tu lon nhat chua ve dung cho");
        require(isPermutation(a, original), "N3 - mat/them phan tu");

        for (int j = 0; j < n - i - 1; ++j) {
            // BAT BIEN TRONG (truoc buoc so sanh j):
            //  (T1) a[j] = max(a[0..j])  -> phan tu lon nhat dang "noi" len
            require(a[j] == maxPrefix(a, j), "T1 - a[j] khong phai max cua a[0..j]");

            if (a[j] > a[j + 1]) swap(a[j], a[j + 1]);

            // Sau buoc j: a[j+1] = max(a[0..j+1])  (bat bien cho j+1)
            require(a[j + 1] == maxPrefix(a, j + 1), "T2 - sau doi cho a[j+1] khong phai max");
        }

        // Sau luot trong: a[n-i-1] la max cua a[0..n-i-1] => bat bien ngoai cho i+1
        require(a[n - i - 1] == maxPrefix(a, n - i - 1), "luot i khong dua max ve vi tri n-i-1");
    }

    // HAU DIEU KIEN
    require(isSortedRange(a, 0, n - 1), "ket qua khong tang dan");
    require(isPermutation(a, original), "ket qua khong la hoan vi cua mang dau");
}

// ---------- Bo test tu dong ----------
static void runTests() {
    mt19937 rng(12345);
    int passed = 0;

    // Truong hop bien
    vector<vector<int>> cases = {
        {}, {5}, {1, 2}, {2, 1}, {3, 3, 3, 3},
        {1, 2, 3, 4, 5},          // da sap xep
        {5, 4, 3, 2, 1},          // nghich dao
        {-3, 0, -3, 7, 7, -1},    // am + trung
        {2147483647, -2147483647 - 1, 0}
    };
    for (auto c : cases) {
        vector<int> expect = c;
        sort(expect.begin(), expect.end());
        bubbleSortChecked(c);
        require(c == expect, "sai ket qua o test bien");
        ++passed;
    }

    // Test ngau nhien
    for (int t = 0; t < 2000; ++t) {
        int n = rng() % 60;
        vector<int> a(n);
        for (int& x : a) x = static_cast<int>(rng() % 41) - 20;
        vector<int> expect = a;
        sort(expect.begin(), expect.end());
        bubbleSortChecked(a);
        require(a == expect, "sai ket qua o test ngau nhien");
        ++passed;
    }
    cout << "Tat ca " << passed << " test DEU DUNG, moi bat bien deu thoa.\n";
}

// ---------- Chuong trinh chinh ----------
int main(int argc, char* argv[]) {
    try {
        if (argc > 1 && string(argv[1]) == "test") {
            runTests();
            return 0;
        }

        long long n;
        cout << "Nhap so phan tu n (0 <= n <= 1000): ";
        if (!(cin >> n) || n < 0 || n > 1000) {
            cerr << "Loi: n khong hop le.\n";
            return 1;
        }

        vector<int> a(static_cast<size_t>(n));
        cout << "Nhap " << n << " so nguyen:\n";
        for (int& x : a) {
            if (!(cin >> x)) {
                cerr << "Loi: du lieu nhap khong hop le.\n";
                return 1;
            }
        }

        bubbleSortChecked(a);

        cout << "Mang sau khi sap xep tang dan:\n";
        for (size_t i = 0; i < a.size(); ++i) cout << a[i] << (i + 1 < a.size() ? ' ' : '\n');
        if (a.empty()) cout << '\n';
        return 0;
    } catch (const exception& e) {
        cerr << e.what() << '\n';
        return 2;
    }
}
