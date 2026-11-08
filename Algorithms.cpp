#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include "Models.h"
#include "ConflictGraph.h"
#include "GreedyScheduler.h"
#include "DPScheduler.h"
#include "BacktrackingScheduler.h"

using namespace std;

// Chạy thuật toán Tham lam & tự động lưu danh sách thí sinh từng phòng kèm đường dẫn
vector<Schedule> runGreedyScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    vector<Schedule> result = GreedyScheduler::schedule(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
    BacktrackingScheduler::exportSchedulesToRoomFiles(result, rooms);
    return result;
}

// Chạy thuật toán Quay lui & tự động lưu danh sách thí sinh từng phòng kèm đường dẫn
vector<Schedule> runBacktrackingScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    return BacktrackingScheduler::schedule(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
}

// Chạy thuật toán Quy hoạch động (DP) & tự động lưu danh sách thí sinh từng phòng kèm đường dẫn
vector<Schedule> runDPScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    vector<Schedule> result = DPScheduler::runDPScheduler(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
    BacktrackingScheduler::exportSchedulesToRoomFiles(result, rooms);
    return result;
}

// ==================== CÁC HÀM GIAO TIẾP VỚI UI ====================

// UI gọi hàm này truyền vào Mã Phòng (VD: "R01") để nhận về đường dẫn file (VD: "data/room_students/R01.txt")
string getRoomStudentFilePath(const string& roomId) {
    return BacktrackingScheduler::getRoomFilePath(roomId);
}

// UI gọi hàm này để lấy toàn bộ Map (Mã Phòng -> Đường dẫn file)
const unordered_map<string, string>& getAllRoomStudentFilePaths() {
    return BacktrackingScheduler::getAllRoomFilePaths();
}

// UI gọi hàm này nếu muốn lấy trực tiếp vector<Student> của phòng đó trên RAM
vector<Student> getStudentsByRoomId(const string& roomId) {
    return BacktrackingScheduler::getStudentsInRoom(roomId);
}
