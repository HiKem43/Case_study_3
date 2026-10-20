#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "Models.h"
#include "ConflictGraph.h"

using namespace std;

// Hàm phụ tạo mã lịch thi tự động tăng: SCH001, SCH002, ..., SCH010
static string makeScheduleId(int n) {
    string num = to_string(n);
    while (num.size() < 3) num = "0" + num; // Đệm thêm số 0 ở đầu cho đủ 3 chữ số
    return "SCH" + num;
}

// Hàm kiểm tra xem độ dài của Ca thi (phút) có đủ cho thời gian làm bài của Môn học không
// VD: Ca thi từ 08:00 đến 10:00 (120 phút) >= Môn C01 thi 90 phút -> Hợp lệ (true)
static bool isSlotDurationEnough(const Course& course, const TimeSlot& slot) {
    int start = timeToMinutes(slot.getStartTime());
    int end = timeToMinutes(slot.getEndTime());
    if (start < 0 || end < 0 || end <= start) return false;
    return (end - start) >= course.getDuration();
}

// Hàm chính xếp lịch thi bằng thuật toán Tham lam.
// Trả về: Danh sách các lịch thi đã xếp thành công (vector<Schedule>).
// Tham số failedCourses: Lưu lại mã các môn KHÔNG xếp được lịch (do hết phòng/hết ca).
vector<Schedule> runGreedyScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    vector<Schedule> result;
    failedCourses.clear();

    // Kiểm tra an toàn đầu vào: Nếu không có môn, hoặc không có phòng/ca thi nào
    // thì ghi nhận toàn bộ môn học vào danh sách thất bại (failedCourses) rồi dừng.
    if (courses.empty() || rooms.empty() || slots.empty()) {
        for (const auto& c : courses) failedCourses.push_back(c.getCourseId());
        return result;
    }

    // Hàm giúp đếm nhanh số lượng sinh viên đăng ký của 1 mã môn
    // VD: studentCount("C01") trả về 6 (nếu môn đó không có trong map thì trả về 0)
    auto studentCount = [&](const string& id) -> size_t {
        auto it = courseStudentsMap.find(id);
        return it == courseStudentsMap.end() ? 0 : it->second.size();
    };

    // b1: dựng đồ thị xung đột
    ConflictGraph graph;
    graph.buildFromData(courseStudentsMap);
    // Đảm bảo 100% môn học trong danh sách courses đều có tên trong đồ thị
    for (const auto& c : courses) graph.addCourse(c.getCourseId());

    // b2: sắp xếp thứ tự ưu tiên
    // 2.1. Sắp xếp các môn học theo thứ tự ưu tiên giảm dần:
    // - Tiêu chí 1: Môn nào xung đột với nhiều môn khác hơn (Bậc cao hơn) thì xếp trước.
    // - Tiêu chí 2: Nếu số xung đột bằng nhau, môn nào đông sinh viên hơn thì xếp trước.
    // - Tiêu chí 3: Nếu bằng nhau cả hai, xếp theo thứ tự bảng chữ cái mã môn (C01 -> C08).
    vector<Course> sortedCourses = courses;
    sort(sortedCourses.begin(), sortedCourses.end(), [&](const Course& a, const Course& b) {
        int da = graph.getDegree(a.getCourseId()); // da = bậc xung đột a tương tự với b
        int db = graph.getDegree(b.getCourseId());
        if (da != db) return da > db;

        size_t sa = studentCount(a.getCourseId()); // sa = học sinh a tượng tự với b
        size_t sb = studentCount(b.getCourseId());
        if (sa != sb) return sa > sb;

        return a.getCourseId() < b.getCourseId();
    });

    // 2.2. Sắp xếp các phòng thi theo sức chứa từ NHỎ đến LỚN:
    // Để môn ít sinh viên tự động chọn phòng nhỏ vừa đủ, nhường phòng lớn cho môn đông sinh viên.
    vector<ExamRoom> sortedRooms = rooms;
    sort(sortedRooms.begin(), sortedRooms.end(), [](const ExamRoom& a, const ExamRoom& b) {
        if (a.getCapacity() != b.getCapacity())
            return a.getCapacity() < b.getCapacity();
        return a.getRoomId() < b.getRoomId();
    });

    // b3: dùng duyệt tham lam để xếp lịch
    // Bảng ghi nhớ mỗi môn đã được xếp vào vị trí ca thi thứ mấy trong mảng slots.
    // VD: slotOfCourse["C01"] = 0 nghĩa là môn C01 đã được xếp vào ca slots[0].
    unordered_map<string, size_t> slotOfCourse;

    // Tập hợp ghi nhớ các mã môn đã được xử lý (chống lỗi dữ liệu đầu vào bị lặp 2 lần cùng 1 môn).
    unordered_set<string> processedCourses;

    // Danh sách ghi nhớ những phòng thi nào đã có lớp dùng ở từng ca.
    // VD: usedRooms[0] chứa {"R01", "R02"} nghĩa là ở ca số 0, phòng R01 và R02 đã kín.
    vector<unordered_set<string>> usedRooms(slots.size());

    int counter = 1; // Biến đếm để đánh số mã lịch thi tăng dần: SCH001, SCH002...

    // Duyệt lần lượt từng môn học theo thứ tự ưu tiên đã sắp xếp ở b2
    for (const auto& course : sortedCourses) {
        string cId = course.getCourseId();

        // Nếu môn này đã được xét rồi thì bỏ qua, đảm bảo 1 môn không bị xếp lịch 2 lần
        if (processedCourses.count(cId)) continue;
        processedCourses.insert(cId);

        if (!course.isValid()) {
            failedCourses.push_back(cId);
            continue;
        }

        int need = (int)studentCount(cId); // Số ghế cần thiết = Số sinh viên đăng ký môn này
        bool assigned = false;             // Cờ đánh dấu xem môn này đã tìm được lịch chưa

        // Thử lần lượt từng Ca thi (s chạy từ ca 0 đến ca cuối cùng)
        for (size_t s = 0; s < slots.size() && !assigned; ++s) {
            if (!slots[s].isValid() || !isSlotDurationEnough(course, slots[s])) continue;

            // 3a. Kiểm tra xung đột sinh viên ở ca thứ s 
            // Duyệt qua tất cả các môn (nb) có chung sinh viên với môn hiện tại (cId).
            // Nếu có môn nb nào đã được xếp lịch rồi, VÀ ca thi của môn nb đó trùng giờ (overlaps)
            // với ca thứ s đang xét -> Sinh viên sẽ bị trùng lịch thi -> Ca s không dùng được!
            bool conflict = false;
            for (const auto& nb : graph.getNeighbors(cId)) {
                auto it = slotOfCourse.find(nb); // Tìm xem môn xung đột nb đã được xếp vào ca nào chưa
                if (it != slotOfCourse.end() && slots[it->second].overlaps(slots[s])) {
                    conflict = true; // trùng giờ thi
                    break;           // stop và chuyển sang ca tt
                }
            }
            if (conflict) continue; // Bỏ qua ca s này vì bị trùng lịch sinh viên

            // 3b. Tìm phòng thi còn trống và đủ chỗ trong ca thứ s
            for (const auto& room : sortedRooms) {
                if (!room.isValid() || room.getCapacity() < need) continue;

                // Kiểm tra xem phòng này đã bị môn khác lấy mất ở ca s (hoặc ca trùng giờ với s) chưa
                bool roomBusy = false;
                for (size_t prevS = 0; prevS < slots.size(); ++prevS) {
                    if (usedRooms[prevS].count(room.getRoomId()) && slots[prevS].overlaps(slots[s])) {
                        roomBusy = true; // Phòng này đã có lớp thi trong khung giờ này
                        break;
                    }
                }
                if (roomBusy) continue; // Thử phòng tiếp theo

                // 3c. Chốt phân công (Đã tìm được Ca s và Phòng room hợp lệ) 
                slotOfCourse[cId] = s;                   // Ghi nhớ: môn cId thi ở ca thứ s
                usedRooms[s].insert(room.getRoomId());   // Đánh dấu ở ca s, phòng này đã có người dùng

                // Tạo đối tượng kết quả Schedule và thêm danh sách sinh viên của môn đó vào phòng thi
                Schedule sch(makeScheduleId(counter++), course, room, slots[s]);
                auto it = courseStudentsMap.find(cId);
                if (it != courseStudentsMap.end())
                    for (const auto& st : it->second) sch.addStudent(st);

                result.push_back(sch); // Đưa lịch thi hoàn chỉnh vào danh sách kết quả
                assigned = true;       // Đánh dấu môn này đã xếp xong
                break;                 // Thoát vòng lặp tìm phòng
            }
        }

        // Nếu đã thử hết mọi Ca thi và Phòng thi mà vẫn không xếp được -> Ghi vào failedCourses
        if (!assigned) failedCourses.push_back(cId);
    }

    return result;
}