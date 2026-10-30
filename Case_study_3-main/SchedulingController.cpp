#include "SchedulingController.h"
#include "Models.h"
#include "GreedyScheduler.h"
#include "BacktrackingScheduler.h"
#include "DPScheduler.h"

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

SchedulingController::SchedulingController() {
    isDataLoaded = false;
}

void SchedulingController::run() {
    int choice;

    do {
        ui.showHeader();
        choice = ui.getMenuChoice();

        switch (choice) {
            // ==========================================
            // CHUC NANG 1: DOC DU LIEU
            // ==========================================
            case 1: {
                cout << "\n[Dang doc du lieu tu file...]\n";

                if (xuLyDuLieu.loadFromFile("data/data_Config")) {
                    xuLyDuLieu.buildConflictGraph();
                    isDataLoaded = true;

                    cout << "Doc du lieu thanh cong!\n";
                    cout << "Da xay dung do thi xung dot.\n";
                } else {
                    isDataLoaded = false;
                    cout << "Doc du lieu that bai!\n";
                    cout << "Hay kiem tra thu muc data.\n";
                }

                ui.pauseScreen();
                break;
            }

            // ==========================================
            // CHUC NANG 2: THUAT TOAN GREEDY
            // ==========================================
            case 2: {
                if (!isDataLoaded) {
                    cout << "\nVui long chon chuc nang 1 de doc du lieu truoc!\n";
                    ui.pauseScreen();
                    break;
                }

                cout << "\n=== THUAT TOAN THAM LAM (GREEDY) ===\n";

                unordered_map<string, vector<Student>> courseStudentsMap;

                for (const auto& student : xuLyDuLieu.getStudents()) {
                    for (const auto& courseId : student.getRegisteredCourses()) {
                        courseStudentsMap[courseId].push_back(student);
                    }
                }

                vector<string> failedCourses;

                vector<Schedule> result = GreedyScheduler::schedule(
                    xuLyDuLieu.getCourses(),
                    xuLyDuLieu.getRooms(),
                    xuLyDuLieu.getTimeSlots(),
                    courseStudentsMap,
                    failedCourses
                );

                cout << "\nSo lich thi da xep: "
                     << result.size() << '\n';

                for (const auto& sch : result) {
                    cout << "Ma lich thi: "
                         << sch.getScheduleId() << '\n';
                }

                if (!failedCourses.empty()) {
                    cout << "\nCac mon chua xep duoc:\n";

                    for (const auto& courseId : failedCourses) {
                        cout << "- " << courseId << '\n';
                    }
                }

                ui.pauseScreen();
                break;
            }

            // ==========================================
// CHUC NANG 3: BACKTRACKING
// ==========================================
case 3: {
    if (!isDataLoaded) {
        cout << "\nVui long chon chuc nang 1 de doc du lieu truoc!\n";
        ui.pauseScreen();
        break;
    }

    cout << "\n=== THUAT TOAN QUAY LUI (BACKTRACKING) ===\n";

    unordered_map<string, vector<Student>> courseStudentsMap;

    for (const auto& student : xuLyDuLieu.getStudents()) {
        for (const auto& courseId : student.getRegisteredCourses()) {
            courseStudentsMap[courseId].push_back(student);
        }
    }

    vector<string> failedCourses;

    vector<Schedule> result = BacktrackingScheduler::schedule(
        xuLyDuLieu.getCourses(),
        xuLyDuLieu.getRooms(),
        xuLyDuLieu.getTimeSlots(),
        courseStudentsMap,
        failedCourses
    );

    cout << "\nSo lich thi da xep: " << result.size() << '\n';

    for (const auto& sch : result) {
        cout << "Ma lich thi: " << sch.getScheduleId() << '\n';
    }

    if (!failedCourses.empty()) {
        cout << "\nCac mon chua xep duoc:\n";
        for (const auto& courseId : failedCourses) {
            cout << "- " << courseId << '\n';
        }
    }

    ui.pauseScreen();
    break;
}

// ==========================================
// CHUC NANG 4: QUY HOACH DONG
// ==========================================
case 4: {
    if (!isDataLoaded) {
        cout << "\nVui long chon chuc nang 1 de doc du lieu truoc!\n";
        ui.pauseScreen();
        break;
    }

    cout << "\n=== THUAT TOAN QUY HOACH DONG (DP) ===\n";

    unordered_map<string, vector<Student>> courseStudentsMap;

    for (const auto& student : xuLyDuLieu.getStudents()) {
        for (const auto& courseId : student.getRegisteredCourses()) {
            courseStudentsMap[courseId].push_back(student);
        }
    }

    vector<string> failedCourses;

    vector<Schedule> result = DPScheduler::runDPScheduler(
        xuLyDuLieu.getCourses(),
        xuLyDuLieu.getRooms(),
        xuLyDuLieu.getTimeSlots(),
        courseStudentsMap,
        failedCourses
    );

    cout << "\nSo lich thi da xep: " << result.size() << '\n';

    for (const auto& sch : result) {
        cout << "Ma lich thi: " << sch.getScheduleId() << '\n';
    }

    if (!failedCourses.empty()) {
        cout << "\nCac mon chua xep duoc:\n";
        for (const auto& courseId : failedCourses) {
            cout << "- " << courseId << '\n';
        }
    }

    ui.pauseScreen();
    break;
}

            // ==========================================
            // CHUC NANG 5: THOAT
            // ==========================================
            case 5: {
                cout << "\nDa thoat chuong trinh.\n";
                break;
            }

            default: {
                cout << "\nLua chon khong hop le! "
                     << "Vui long chon tu 1 den 5.\n";
                ui.pauseScreen();
                break;
            }
        }

    } while (choice != 5);
}