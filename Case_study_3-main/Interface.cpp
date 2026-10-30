#include "Interface.h"

#include <iostream>
#include <limits>

using namespace std;

void Interface::showHeader() {
    cout << "\n";
    cout << "============================================\n";
    cout << "          CHUONG TRINH XEP LICH THI         \n";
    cout << "============================================\n";
}

int Interface::getMenuChoice() {
    int choice;

    cout << "\n--------------- MENU CHUC NANG -------------\n";
    cout << "1. Doc du lieu dau vao\n";
    cout << "2. Chay thuat toan Tham lam (Greedy)\n";
    cout << "3. Chay thuat toan Quay lui (Backtracking)\n";
    cout << "4. Chay thuat toan Quy hoach dong (DP)\n";
    cout << "5. Thoat chuong trinh\n";
    cout << "--------------------------------------------\n";
    cout << "Nhap lua chon cua ban (1-5): ";

    while (!(cin >> choice) || choice < 1 || choice > 5) {
        cout << "Lua chon khong hop le. Vui long nhap tu 1 den 5: ";

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Loai bo phan con lai tren dong vua nhap
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    return choice;
}

void Interface::pauseScreen() {
    cout << "\nNhan Enter de quay lai menu...";

    // Cho nguoi dung nhan Enter
    cin.get();
}