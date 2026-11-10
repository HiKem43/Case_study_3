#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <limits>
#include <algorithm>

using namespace std;

const int MIN_N = 2;
const int MAX_N = 15;

class TSPBacktracking {
private:
    int n;
    vector<vector<long long>> cost;   // Ma tran chi phi C[i][j]
    vector<long long> minOut;         // minOut[i]: chi phi canh ra nho nhat cua thanh pho i
    vector<bool> visited;
    vector<int> path;                 // Lo trinh hien tai
    vector<int> bestPath;             // Lo trinh tot nhat
    long long bestCost;               // Chi phi toi thieu tim duoc

    // curr: thanh pho hien tai, count: so thanh pho da tham,
    // curCost: chi phi tich luy, unvisitedMin: tong minOut cua cac thanh pho chua tham
    void backtrack(int curr, int count, long long curCost, long long unvisitedMin) {
        // Da tham du n thanh pho -> quay ve thanh pho 0
        if (count == n) {
            long long total = curCost + cost[curr][0];
            if (total < bestCost) {
                bestCost = total;
                bestPath = path;
                bestPath.push_back(0);
            }
            return;
        }

        // [CAT TIA 2] Can duoi: chi phi hien tai + canh ra nho nhat cua thanh pho hien tai
        // va cua moi thanh pho chua tham. Neu da >= ky luc thi bo nhanh.
        if (curCost + minOut[curr] + unvisitedMin >= bestCost) {
            return;
        }

        // Thu di tiep den tung thanh pho chua tham
        for (int next = 0; next < n; ++next) {
            if (visited[next]) continue;

            long long newCost = curCost + cost[curr][next];
            if (newCost >= bestCost) continue; // [CAT TIA 1] chi phi da vuot ky luc

            // --- THU ---
            visited[next] = true;
            path.push_back(next);

            backtrack(next, count + 1, newCost, unvisitedMin - minOut[next]);

            // --- QUAY LUI ---
            path.pop_back();
            visited[next] = false;
        }
    }

public:
    explicit TSPBacktracking(const vector<vector<long long>>& matrix)
        : n((int)matrix.size()), cost(matrix) {
        minOut.assign(n, numeric_limits<long long>::max());
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) minOut[i] = min(minOut[i], cost[i][j]);
            }
        }
        visited.assign(n, false);
        bestCost = numeric_limits<long long>::max();
    }

    void solve() {
        // Xuat phat tu thanh pho 0
        visited[0] = true;
        path.push_back(0);

        long long unvisitedMin = 0;
        for (int i = 1; i < n; ++i) unvisitedMin += minOut[i];

        backtrack(0, 1, 0, unvisitedMin);
    }

    long long getBestCost() const { return bestCost; }
    const vector<int>& getBestPath() const { return bestPath; }
};

// Bo qua phan con lai cua dong hien tai sau khi nhap sai
static void discardLine() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Doc n, nhap sai thi bao loi va yeu cau nhap lai. Tra ve false neu het du lieu (EOF).
static bool readN(int& n) {
    while (true) {
        cerr << "Nhap so thanh pho n (" << MIN_N << " <= n <= " << MAX_N << "): ";
        if (!(cin >> n)) {
            if (cin.eof()) return false;
            discardLine();
            cerr << "Loi: n phai la so nguyen. Hay nhap lai.\n";
            continue;
        }
        if (n < MIN_N || n > MAX_N) {
            discardLine();
            cerr << "Loi: n phai nam trong khoang [" << MIN_N << ", " << MAX_N << "]. Hay nhap lai.\n";
            continue;
        }
        return true;
    }
}

// Doc ma tran chi phi n x n tung hang. Hang nhap sai se bi yeu cau nhap lai.
static bool readMatrix(int n, vector<vector<long long>>& matrix) {
    cerr << "Nhap ma tran chi phi " << n << " x " << n
         << " (moi hang " << n << " so nguyen khong am, C[i][j] la chi phi di tu i den j):\n";

    matrix.assign(n, vector<long long>(n));
    for (int i = 0; i < n; ++i) {
        while (true) {
            cerr << "  Hang " << i << ": ";
            vector<long long> row(n);
            bool ok = true;
            for (int j = 0; j < n; ++j) {
                if (!(cin >> row[j])) {
                    if (cin.eof()) return false;
                    ok = false;
                    break;
                }
                if (row[j] < 0) ok = false; // chi phi am lam cat tia sai nen khong chap nhan
            }
            if (ok) {
                matrix[i] = row;
                break;
            }
            discardLine();
            cerr << "  Loi: hang " << i << " phai co dung " << n
                 << " so nguyen khong am. Hay nhap lai hang nay.\n";
        }
    }
    return true;
}

int main() {
    int n;
    if (!readN(n)) {
        cerr << "Khong doc duoc n (het du lieu dau vao).\n";
        return 1;
    }

    vector<vector<long long>> matrix;
    if (!readMatrix(n, matrix)) {
        cerr << "Khong doc du ma tran (het du lieu dau vao).\n";
        return 1;
    }

    TSPBacktracking solver(matrix);

    auto start = chrono::steady_clock::now();
    solver.solve();
    double elapsed = chrono::duration<double, milli>(
        chrono::steady_clock::now() - start
    ).count();

    cout << "====BACKTRACKING====" << endl;
    cout << "Chi phi toi thieu: " << solver.getBestCost() << endl;
    cout << "Lo trinh:";
    for (int city : solver.getBestPath()) cout << " " << city;
    cout << endl;
    cout << "Thoi gian: " << fixed << setprecision(3) << elapsed << " ms" << endl;

    return 0;
}