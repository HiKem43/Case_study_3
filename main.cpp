#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include "DataProcessor.h"
#include "GreedyScheduler.h"
#include "DPScheduler.h"
#include "BacktrackingScheduler.h"

using namespace std;

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

int main() {
    DataProcessor dataProcessor;

    if (!dataProcessor.loadFromFile("data/data_Config.txt")) {
        cerr << "Khong the doc du lieu!" << endl;
        return 1;
    }

    if (!dataProcessor.validateData()) {
        cerr << "Du lieu khong hop le. Hay kiem tra lai!" << endl;
        return 1;
    }

    dataProcessor.buildConflictGraph();

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

    vector<string> failedCourses;
    auto start = chrono::steady_clock::now();
    vector<Schedule> greedySchedules = GreedyScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, failedCourses
    );
    auto elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults("Thuat toan tham lam (Greedy)", greedySchedules, failedCourses, elapsed);

    start = chrono::steady_clock::now();
    vector<Schedule> dpSchedules = DPScheduler::runDPScheduler(
        courses, rooms, timeSlots, courseStudentsMap, failedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults("Quy hoach dong (DP)", dpSchedules, failedCourses, elapsed);

    start = chrono::steady_clock::now();
    vector<Schedule> backtrackingSchedules = BacktrackingScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, failedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    printResults(
        "Quay lui va cat tia (Backtracking)",
        backtrackingSchedules,
        failedCourses,
        elapsed
    );

    return 0;
}
