#include <iostream>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include "DataProcessor.h"
#include "GreedyScheduler.h"
#include "DPScheduler.h"
#include "BacktrackingScheduler.h"

using namespace std;
namespace fs = std::filesystem;

static bool setWorkingDirectoryToDataRoot() {
    const fs::path configRelativePath = fs::path("data") / "data_Config.txt";
    error_code error;

    auto isRegularFile = [&error](const fs::path& path) {
        error.clear();
        const bool regularFile = fs::is_regular_file(path, error);
        if (error == errc::no_such_file_or_directory || error == errc::not_a_directory) {
            error.clear();
        }
        return regularFile;
    };

    fs::path directory = fs::current_path(error);
    if (error) {
        cerr << "Khong the xac dinh thu muc lam viec: " << error.message() << endl;
        return false;
    }

    while (!directory.empty()) {
        const fs::path candidate = directory / configRelativePath;
        if (isRegularFile(candidate)) {
            fs::current_path(directory, error);
            if (error) {
                cerr << "Khong the chuyen den thu muc du lieu: " << error.message() << endl;
                return false;
            }
            return true;
        }
        if (error) {
            cerr << "Khong the kiem tra file du lieu: " << error.message() << endl;
            return false;
        }

        const fs::path parent = directory.parent_path();
        if (parent == directory) {
            break;
        }
        directory = parent;
    }

    cerr << "Khong tim thay data/data_Config.txt trong thu muc hien tai "
         << "hoac cac thu muc cha." << endl;
    return false;
}

void printResults(
    const string& algorithmName,
    const vector<Schedule>& schedules,
    const vector<string>& failedCourses,
    long long elapsedMicroseconds
) {
    cout << "\n=== " << algorithmName << " ===" << endl;
    cout << "So mon da xep: " << schedules.size() << endl;
    cout << "Thoi gian: " << elapsedMicroseconds << " micro giay" << endl;

    for (const Schedule& schedule : schedules) {
        const Course course = schedule.getCourse();
        const ExamRoom room = schedule.getRoom();
        const TimeSlot slot = schedule.getTimeSlot();

        cout << schedule.getScheduleId()
             << " | " << course.getCourseId()
             << " - " << course.getCourseName()
             << " | " << slot.getExamDate()
             << " " << slot.getStartTime() << "-" << slot.getEndTime()
             << " | Phong " << room.getRoomId()
             << " | " << schedule.getStudents().size() << " sinh vien"
             << endl;
    }

    if (!failedCourses.empty()) {
        cout << "Cac mon chua xep duoc: ";
        for (size_t i = 0; i < failedCourses.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << failedCourses[i];
        }
        cout << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        cerr << "Cach dung: " << argv[0]
             << " [duong_dan_file_config]" << endl;
        return 1;
    }

    string configFile = "data/data_Config.txt";
    if (argc == 2) {
        configFile = argv[1];
    } else if (!setWorkingDirectoryToDataRoot()) {
        return 1;
    }

    DataProcessor dataProcessor;

    if (!dataProcessor.loadFromFile(configFile)) {
        cerr << "Khong the doc du lieu!" << endl;
        return 1;
    }

    if (!dataProcessor.validateData()) {
        cerr << "Du lieu khong hop le. Hay kiem tra lai!" << endl;
        return 1;
    }


    const vector<Course> courses = dataProcessor.getCourses();
    const vector<Student> students = dataProcessor.getStudents();
    const vector<ExamRoom> rooms = dataProcessor.getRooms();
    const vector<TimeSlot> timeSlots = dataProcessor.getTimeSlots();

    unordered_map<string, vector<Student>> courseStudentsMap;
    for (const Student& student : students) {
        for (const string& courseId : student.getRegisteredCourses()) {
            courseStudentsMap[courseId].push_back(student);
        }
    }

    cout << "Du lieu hop le. Bat dau chay cac thuat toan xep lich." << endl;

    vector<string> greedyFailedCourses;
    auto start = chrono::steady_clock::now();
    vector<Schedule> greedySchedules = GreedyScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, greedyFailedCourses
    );
    auto elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults("Thuat toan tham lam (Greedy)", greedySchedules, greedyFailedCourses, elapsed);

    vector<string> dpFailedCourses;
    start = chrono::steady_clock::now();
    vector<Schedule> dpSchedules = DPScheduler::runDPScheduler(
        courses, rooms, timeSlots, courseStudentsMap, dpFailedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults("Quy hoach dong (DP)", dpSchedules, dpFailedCourses, elapsed);

    vector<string> backtrackingFailedCourses;
    start = chrono::steady_clock::now();
    vector<Schedule> backtrackingSchedules = BacktrackingScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, backtrackingFailedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults(
        "Quay lui va cat tia (Backtracking)",
        backtrackingSchedules,
        backtrackingFailedCourses,
        elapsed
    );

    return 0;
}
