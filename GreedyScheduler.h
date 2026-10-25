#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "Models.h"
#include "ConflictGraph.h"

using namespace std;

class GreedyScheduler {
private:
    // Tạo mã lịch thi tự động tăng: SCH001, SCH002... 
    static string makeScheduleId(int n) {
        string num = to_string(n);
        while (num.size() < 3) num = "0" + num;
        return "SCH" + num;
    }

    // Kiểm tra thời lượng ca thi có đủ cho thời gian làm bài của môn học không
    static bool isSlotDurationEnough(const Course& course, const TimeSlot& slot) {
        int start = timeToMinutes(slot.getStartTime());
        int end = timeToMinutes(slot.getEndTime());
        if (start < 0 || end < 0 || end <= start) return false;
        return (end - start) >= course.getDuration();
    }

public:
    // Hàm chính thực thi thuật toán Tham lam
    static vector<Schedule> schedule(
        const vector<Course>& courses,
        const vector<ExamRoom>& rooms,
        const vector<TimeSlot>& slots,
        const unordered_map<string, vector<Student>>& courseStudentsMap,
        vector<string>& failedCourses
    ) {
        vector<Schedule> result;
        failedCourses.clear();

        if (courses.empty() || rooms.empty() || slots.empty()) {
            for (const auto& c : courses) failedCourses.push_back(c.getCourseId());
            return result;
        }

        auto studentCount = [&](const string& id) -> size_t {
            auto it = courseStudentsMap.find(id);
            return it == courseStudentsMap.end() ? 0 : it->second.size();
        };

        // dựng đồ thị xung đột
        ConflictGraph graph;
        graph.buildFromData(courseStudentsMap);
        for (const auto& c : courses) {
            graph.addCourse(c.getCourseId());
        }

        // sắp xếp thứ tự ưu tiên
        vector<Course> sortedCourses = courses;
        sort(sortedCourses.begin(), sortedCourses.end(), [&](const Course& a, const Course& b) {
            int da = graph.getDegree(a.getCourseId());
            int db = graph.getDegree(b.getCourseId());
            if (da != db) return da > db; // Ưu tiên bậc xung đột cao hơn xếp trước

            size_t sa = studentCount(a.getCourseId());
            size_t sb = studentCount(b.getCourseId());
            if (sa != sb) return sa > sb; // Bằng bậc: ưu tiên môn đông sinh viên hơn

            return a.getCourseId() < b.getCourseId(); // Bằng cả hai: xếp theo mã A-Z
        });

        // Sắp xếp phòng thi từ nhỏ đến lớn để tiết kiệm phòng lớn
        vector<ExamRoom> sortedRooms = rooms;
        sort(sortedRooms.begin(), sortedRooms.end(), [](const ExamRoom& a, const ExamRoom& b) {
            if (a.getCapacity() != b.getCapacity())
                return a.getCapacity() < b.getCapacity();
            return a.getRoomId() < b.getRoomId();
        });

        // Duyệt tham lam sắp xếp lịch
        unordered_map<string, size_t> slotOfCourse;            // Lưu vị trí ca thi đã gán của mỗi môn
        unordered_set<string> processedCourses;                // Chống xếp trùng 1 môn nhiều lần
        vector<unordered_set<string>> usedRooms(slots.size()); // Lưu các mã phòng đã dùng ở từng ca
        int counter = 1;

        for (const auto& course : sortedCourses) {
            string cId = course.getCourseId();
            if (processedCourses.count(cId)) continue;
            processedCourses.insert(cId);

            if (!course.isValid()) {
                failedCourses.push_back(cId);
                continue;
            }

            int need = (int)studentCount(cId);
            bool assigned = false;

            for (size_t s = 0; s < slots.size() && !assigned; ++s) {
                if (!slots[s].isValid() || !isSlotDurationEnough(course, slots[s])) continue;

                // Kiểm tra xung đột sinh viên ở ca s (và các ca giao giờ với s)
                bool conflict = false;
                for (const auto& nb : graph.getNeighbors(cId)) {
                    auto it = slotOfCourse.find(nb);
                    if (it != slotOfCourse.end() && slots[it->second].overlaps(slots[s])) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;

                // Tìm phòng trống và đủ sức chứa trong ca s
                for (const auto& room : sortedRooms) {
                    if (!room.isValid() || room.getCapacity() < need) continue;

                    bool roomBusy = false;
                    for (size_t prevS = 0; prevS < slots.size(); ++prevS) {
                        if (usedRooms[prevS].count(room.getRoomId()) && slots[prevS].overlaps(slots[s])) {
                            roomBusy = true;
                            break;
                        }
                    }
                    if (roomBusy) continue;

                    //  Chốt phân công
                    slotOfCourse[cId] = s;
                    usedRooms[s].insert(room.getRoomId());

                    Schedule sch(makeScheduleId(counter++), course, room, slots[s]);
                    auto it = courseStudentsMap.find(cId);
                    if (it != courseStudentsMap.end()) {
                        for (const auto& st : it->second) sch.addStudent(st);
                    }

                    result.push_back(sch);
                    assigned = true;
                    break;
                }
            }

            if (!assigned) failedCourses.push_back(cId);
        }

        return result;
    }
};