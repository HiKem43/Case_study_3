#include <iostream>
#include "DataProcessor.h"
using namespace std;

int main() {
    DataProcessor xuLyDuLieu;

    // Buoc 1: Doc cac file trong thu muc data
    if (!xuLyDuLieu.loadFromFile("data/data_Config")) {
        cout << "Khong the doc du lieu!" << endl;
        return 1;
    }

    // Buoc 2: Kiem tra tinh hop le
    if (!xuLyDuLieu.validateData()) {
        cout << "Du lieu khong hop le. Hay kiem tra lai!"
             << endl;
        return 1;
    }

    // Buoc 3: Xay dung do thi xung dot
    xuLyDuLieu.buildConflictGraph();

    cout << "Du lieu hop le!" << endl;
    cout << "Da xay dung do thi xung dot." << endl;

    return 0;
}
