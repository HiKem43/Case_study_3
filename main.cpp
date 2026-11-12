#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "DataProcessor.h"
#include "GreedyScheduler.h"
#include "DPScheduler.h"
#include "BacktrackingScheduler.h"

using namespace std;
namespace fs = std::filesystem;

void printResults(
    ostream& output,
    const string& algorithmName,
    const vector<Schedule>& schedules,
    const vector<string>& failedCourses,
    long long elapsedMicroseconds
) {
    output << "\n=== " << algorithmName << " ===\n";
    output << "Số môn đã xếp: " << schedules.size() << '\n';
    output << "Thời gian: " << elapsedMicroseconds << " micro giây\n";

    for (const Schedule& schedule : schedules) {
        const Course course = schedule.getCourse();
        const ExamRoom room = schedule.getRoom();
        const TimeSlot slot = schedule.getTimeSlot();

        output << schedule.getScheduleId()
             << " | " << course.getCourseId()
             << " - " << course.getCourseName()
             << " | " << slot.getExamDate()
             << " " << slot.getStartTime() << "-" << slot.getEndTime()
             << " | Phòng " << room.getRoomId()
             << " | " << schedule.getStudents().size() << " sinh viên"
             << '\n';
    }

    if (!failedCourses.empty()) {
        output << "Các môn chưa xếp được: ";
        for (size_t i = 0; i < failedCourses.size(); ++i) {
            if (i > 0) output << ", ";
            output << failedCourses[i];
        }
        output << '\n';
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    const HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    if (consoleOutput != INVALID_HANDLE_VALUE &&
        GetConsoleMode(consoleOutput, &consoleMode) &&
        !SetConsoleOutputCP(CP_UTF8)) {
        const DWORD errorCode = GetLastError();
        cerr << "Không thể đặt mã hóa console thành UTF-8. Mã lỗi: "
             << errorCode << '\n';
        return 1;
    }
#endif

    if (argc > 2) {
        cerr << "Cách dùng: " << argv[0]
             << " [thu_muc_chua_du_lieu]\n";
        return 1;
    }

    const fs::path inputDirectory = argc == 2 ? argv[1] : "input";

    DataProcessor dataProcessor;

    if (!dataProcessor.loadFromFiles(
            inputDirectory / "data_Config.txt",
            inputDirectory / "data_Courses.txt",
            inputDirectory / "data_Students.txt",
            inputDirectory / "data_Rooms.txt",
            inputDirectory / "data_Time_Slots.txt"
        )) {
        cerr << "Không thể tải dữ liệu!\n";
        return 1;
    }

    if (!dataProcessor.validateData()) {
        cerr << "Dữ liệu không hợp lệ. Hãy kiểm tra lại!\n";
        return 1;
    }

    error_code outputError;
    fs::create_directories("output", outputError);
    if (outputError) {
        cerr << "Không thể tạo thư mục output: "
             << outputError.message() << '\n';
        return 1;
    }

    ofstream outputFile(fs::path("output") / "output.txt");
    if (!outputFile) {
        cerr << "Không thể mở file output/output.txt để ghi.\n";
        return 1;
    }
    outputFile.write("\xEF\xBB\xBF", 3);

    const vector<Course> courses = dataProcessor.getCourses();
    const vector<Student> students = dataProcessor.getStudents();
    const vector<ExamRoom> rooms = dataProcessor.getRooms();
    const vector<TimeSlot> timeSlots = dataProcessor.getTimeSlots();

    unordered_map<string, vector<Student>> courseStudentsMap;
    courseStudentsMap.reserve(courses.size());
    for (const Student& student : students) {
        for (const string& courseId : student.getRegisteredCourses()) {
            courseStudentsMap[courseId].push_back(student);
        }
    }

    cout << "Dữ liệu hợp lệ. Bắt đầu chạy các thuật toán xếp lịch.\n";
    outputFile << "Dữ liệu hợp lệ. Bắt đầu chạy các thuật toán xếp lịch.\n";

    auto reportResults = [&](const string& algorithmName,
                             const vector<Schedule>& schedules,
                             const vector<string>& failedCourses,
                             long long elapsedMicroseconds) {
        printResults(cout, algorithmName, schedules, failedCourses, elapsedMicroseconds);
        printResults(outputFile, algorithmName, schedules, failedCourses, elapsedMicroseconds);
    };

    vector<string> greedyFailedCourses;
    auto start = chrono::steady_clock::now();
    vector<Schedule> greedySchedules = GreedyScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, greedyFailedCourses
    );
    auto elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    reportResults("Thuật toán tham lam (Greedy)", greedySchedules, greedyFailedCourses, elapsed);

    vector<string> dpFailedCourses;
    start = chrono::steady_clock::now();
    vector<Schedule> dpSchedules = DPScheduler::runDPScheduler(
        courses, rooms, timeSlots, courseStudentsMap, dpFailedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    reportResults("Quy hoạch động (DP)", dpSchedules, dpFailedCourses, elapsed);

    vector<string> backtrackingFailedCourses;
    start = chrono::steady_clock::now();
    vector<Schedule> backtrackingSchedules = BacktrackingScheduler::schedule(
        courses, rooms, timeSlots, courseStudentsMap, backtrackingFailedCourses
    );
    elapsed = chrono::duration_cast<chrono::microseconds>(
        chrono::steady_clock::now() - start
    ).count();
    reportResults(
        "Quay lui (Backtracking)",
        backtrackingSchedules,
        backtrackingFailedCourses,
        elapsed
    );

    return 0;
}
