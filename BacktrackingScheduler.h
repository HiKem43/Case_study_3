#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include "Models.h"
#include "ConflictGraph.h"

using namespace std;

class BacktrackingScheduler {
private:
    vector<Course> validCourses;
    vector<ExamRoom> sortedRooms;
    const vector<TimeSlot>& slots;
    const unordered_map<string, vector<Student>>& courseStudentsMap;
    ConflictGraph graph;

    int numCourses;
    int numRooms;
    int numSlots;

    // Trạng thái hiện tại trong quá trình Quay lui
    vector<int> currentSlot;
    vector<int> currentRoom;

    // Lưu nghiệm tốt nhất (Best Solution) phục vụ Cắt tỉa (Pruning)
    vector<int> bestSlot;
    vector<int> bestRoom;
    int bestAssignedCount;
    int bestMaxSlotIndex;

    // ==================== BỘ LƯU TRỮ TRUY XUẤT CHO GIAO DIỆN (UI) ====================
    // Sử dụng inline static để lưu giữ dữ liệu và đường dẫn ngay sau khi schedule() chạy xong
    inline static unordered_map<string, string> roomFilePaths;               // Map: Mã Phòng -> Đường dẫn file
    inline static unordered_map<string, vector<Student>> roomStudentsMap;    // Map: Mã Phòng -> Danh sách thí sinh
    inline static unordered_map<string, vector<Schedule>> roomSchedulesMap;  // Map: Mã Phòng -> Các ca thi tại phòng đó

    // Tự động lưu dữ liệu từng phòng ra file và cập nhật đường dẫn truy xuất
    static void saveRoomStudentData(
        const vector<Schedule>& schedules,
        const vector<ExamRoom>& allRooms,
        const string& baseDir = "data/room_students"
    ) {
        namespace fs = std::filesystem;
        roomFilePaths.clear();
        roomStudentsMap.clear();
        roomSchedulesMap.clear();

        error_code ec;
        fs::create_directories(baseDir, ec);

        // 1. Gom nhóm lịch thi và thí sinh theo từng phòng trên RAM
        for (const auto& sch : schedules) {
            string rId = normalizeCode(sch.getRoom().getRoomId());
            roomSchedulesMap[rId].push_back(sch);

            vector<Student> sts = sch.getStudents();
            for (const auto& st : sts) {
                roomStudentsMap[rId].push_back(st);
            }
        }

        // 2. Ghi file cho từng phòng và lưu lại đường dẫn
        for (const auto& room : allRooms) {
            string rId = normalizeCode(room.getRoomId());
            string filePath = (fs::path(baseDir) / (rId + ".txt")).generic_string();
            roomFilePaths[rId] = filePath;

            ofstream out(filePath.c_str());
            if (!out.is_open()) continue;

            out << "ROOM|" << rId << "|CAPACITY|" << room.getCapacity() << "\n";

            auto it = roomSchedulesMap.find(rId);
            if (it != roomSchedulesMap.end()) {
                for (const auto& sch : it->second) {
                    Course c = sch.getCourse();
                    TimeSlot t = sch.getTimeSlot();
                    vector<Student> students = sch.getStudents();

                    out << "SCHEDULE|" << sch.getScheduleId()
                        << "|" << t.getTimeSlotId()
                        << "|" << t.getExamDate()
                        << "|" << t.getStartTime() << "-" << t.getEndTime()
                        << "|" << c.getCourseId()
                        << "|" << c.getCourseName()
                        << "|" << students.size() << "\n";

                    for (const auto& st : students) {
                        out << "STUDENT|" << st.getStudentId()
                            << "|" << st.getFullName()
                            << "|" << st.getDateOfBirth()
                            << "|" << st.getGender() << "\n";
                    }
                }
            }
            out.close();
        }

        // 3. Tạo file index_paths.txt tổng hợp: Mã_Phòng|Đường_Dẫn
        string indexPath = (fs::path(baseDir) / "index_paths.txt").generic_string();
        ofstream idxOut(indexPath.c_str());
        if (idxOut.is_open()) {
            for (const auto& room : allRooms) {
                string rId = normalizeCode(room.getRoomId());
                idxOut << rId << "|" << roomFilePaths[rId] << "\n";
            }
            idxOut.close();
        }
    }

    // Tạo mã lịch thi tự động tăng: SCH001, SCH002... (đồng bộ với GreedyScheduler)
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

    // Lấy số lượng sinh viên đăng ký môn học
    int getStudentCount(const string& courseId) const {
        auto it = courseStudentsMap.find(courseId);
        return it == courseStudentsMap.end() ? 0 : (int)it->second.size();
    }

    // 1. KIỂM TRA CA THI HỢP LỆ & TÌM PHÒNG PHÙ HỢP
    int findValidRoomForSlot(int courseIdx, int slotIdx) const {
        const Course& course = validCourses[courseIdx];
        const TimeSlot& slot = slots[slotIdx];

        if (!slot.isValid() || !isSlotDurationEnough(course, slot)) {
            return -1;
        }

        string cId = course.getCourseId();

        for (int i = 0; i < courseIdx; ++i) {
            int prevSlot = currentSlot[i];
            if (prevSlot != -1 && slots[prevSlot].overlaps(slot)) {
                if (graph.hasConflict(cId, validCourses[i].getCourseId())) {
                    return -1;
                }
            }
        }

        int need = getStudentCount(cId);

        for (int r = 0; r < numRooms; ++r) {
            if (!sortedRooms[r].isValid() || sortedRooms[r].getCapacity() < need) {
                continue;
            }

            bool roomBusy = false;
            for (int i = 0; i < courseIdx; ++i) {
                if (currentSlot[i] != -1 && currentRoom[i] == r) {
                    if (slots[currentSlot[i]].overlaps(slot)) {
                        roomBusy = true;
                        break;
                    }
                }
            }

            if (!roomBusy) {
                return r;
            }
        }

        return -1;
    }

    // 2. HÀM ĐỆ QUY, QUAY LUI VÀ CẮT TỈA
    void backtrack(int courseIdx, int assignedCount, int maxSlotUsed) {
        int remaining = numCourses - courseIdx;

        if (assignedCount + remaining < bestAssignedCount) {
            return;
        }

        if (assignedCount + remaining == bestAssignedCount && maxSlotUsed >= bestMaxSlotIndex) {
            return;
        }

        if (courseIdx == numCourses) {
            if (assignedCount > bestAssignedCount ||
                (assignedCount == bestAssignedCount && maxSlotUsed < bestMaxSlotIndex)) {
                bestAssignedCount = assignedCount;
                bestMaxSlotIndex = maxSlotUsed;
                bestSlot = currentSlot;
                bestRoom = currentRoom;
            }
            return;
        }

        for (int s = 0; s < numSlots; ++s) {
            int nextMaxSlot = max(maxSlotUsed, s + 1);

            if (assignedCount + remaining == bestAssignedCount && nextMaxSlot >= bestMaxSlotIndex) {
                continue;
            }

            int r = findValidRoomForSlot(courseIdx, s);
            if (r != -1) {
                currentSlot[courseIdx] = s;
                currentRoom[courseIdx] = r;

                backtrack(courseIdx + 1, assignedCount + 1, nextMaxSlot);

                currentSlot[courseIdx] = -1;
                currentRoom[courseIdx] = -1;
            }
        }

        if (assignedCount + remaining - 1 >= bestAssignedCount) {
            currentSlot[courseIdx] = -1;
            currentRoom[courseIdx] = -1;
            backtrack(courseIdx + 1, assignedCount, maxSlotUsed);
        }
    }

public:
    BacktrackingScheduler(
        const vector<Course>& courses,
        const vector<ExamRoom>& rooms,
        const vector<TimeSlot>& slots,
        const unordered_map<string, vector<Student>>& courseStudentsMap,
        vector<string>& invalidOrDuplicateFailed
    ) : slots(slots), courseStudentsMap(courseStudentsMap) {

        graph.buildFromData(courseStudentsMap);
        for (const auto& c : courses) {
            graph.addCourse(c.getCourseId());
        }

        unordered_set<string> processedCourses;
        for (const auto& c : courses) {
            string cId = c.getCourseId();
            if (processedCourses.count(cId)) continue;
            processedCourses.insert(cId);

            if (!c.isValid()) {
                invalidOrDuplicateFailed.push_back(cId);
                continue;
            }
            validCourses.push_back(c);
        }

        sort(validCourses.begin(), validCourses.end(), [&](const Course& a, const Course& b) {
            int da = graph.getDegree(a.getCourseId());
            int db = graph.getDegree(b.getCourseId());
            if (da != db) return da > db;

            int sa = getStudentCount(a.getCourseId());
            int sb = getStudentCount(b.getCourseId());
            if (sa != sb) return sa > sb;

            return a.getCourseId() < b.getCourseId();
        });

        sortedRooms = rooms;
        sort(sortedRooms.begin(), sortedRooms.end(), [](const ExamRoom& a, const ExamRoom& b) {
            if (a.getCapacity() != b.getCapacity())
                return a.getCapacity() < b.getCapacity();
            return a.getRoomId() < b.getRoomId();
        });

        numCourses = (int)validCourses.size();
        numRooms = (int)sortedRooms.size();
        numSlots = (int)slots.size();

        currentSlot.assign(numCourses, -1);
        currentRoom.assign(numCourses, -1);
        bestSlot.assign(numCourses, -1);
        bestRoom.assign(numCourses, -1);

        bestAssignedCount = -1;
        bestMaxSlotIndex = numSlots + 1;
    }

    // Hàm tĩnh gọi thực thi thuật toán Quay lui & Cắt tỉa
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

        BacktrackingScheduler solver(courses, rooms, slots, courseStudentsMap, failedCourses);
        solver.backtrack(0, 0, 0);

        int counter = 1;
        for (int i = 0; i < solver.numCourses; ++i) {
            const Course& course = solver.validCourses[i];
            string cId = course.getCourseId();

            int sIdx = solver.bestSlot[i];
            int rIdx = solver.bestRoom[i];

            if (sIdx != -1 && rIdx != -1) {
                Schedule sch(makeScheduleId(counter++), course, solver.sortedRooms[rIdx], slots[sIdx]);
                auto it = courseStudentsMap.find(cId);
                if (it != courseStudentsMap.end()) {
                    for (const auto& st : it->second) {
                        sch.addStudent(st);
                    }
                }
                result.push_back(sch);
            }
            else {
                failedCourses.push_back(cId);
            }
        }

        // TỰ ĐỘNG LƯU DANH SÁCH THÍ SINH TỪNG PHÒNG VÀ ĐƯỜNG DẪN TRUY XUẤT NGAY KHI XẾP XONG
        saveRoomStudentData(result, rooms);

        return result;
    }

    // ==================== CÁC HÀM ĐỂ UI LẤY ĐƯỜNG DẪN & THÍ SINH ====================

    // 1. Lấy đường dẫn file chứa danh sách thí sinh của 1 phòng (VD: truyền "R01" -> trả về "data/room_students/R01.txt")
    static string getRoomFilePath(const string& roomId) {
        string rId = normalizeCode(roomId);
        auto it = roomFilePaths.find(rId);
        if (it != roomFilePaths.end()) {
            return it->second;
        }
        return "data/room_students/" + rId + ".txt";
    }

    // 2. Lấy toàn bộ bảng ánh xạ (Mã Phòng -> Đường dẫn file)
    static const unordered_map<string, string>& getAllRoomFilePaths() {
        return roomFilePaths;
    }

    // 3. Lấy trực tiếp danh sách thí sinh của 1 phòng trên RAM (nếu UI không muốn đọc lại từ file)
    static vector<Student> getStudentsInRoom(const string& roomId) {
        string rId = normalizeCode(roomId);
        auto it = roomStudentsMap.find(rId);
        return it == roomStudentsMap.end() ? vector<Student>{} : it->second;
    }

    // 4. Lấy danh sách thí sinh của 1 phòng tại 1 ca thi cụ thể
    static vector<Student> getStudentsInRoomAtSlot(const string& roomId, const string& timeSlotId) {
        string rId = normalizeCode(roomId);
        string sId = normalizeCode(timeSlotId);
        auto it = roomSchedulesMap.find(rId);
        if (it != roomSchedulesMap.end()) {
            for (const auto& sch : it->second) {
                if (sch.getTimeSlot().getTimeSlotId() == sId) {
                    return sch.getStudents();
                }
            }
        }
        return {};
    }

    // 5. Cho phép lưu danh sách thí sinh của bất kỳ lịch thi nào (Greedy/DP) vào chung hệ thống đường dẫn
    static void exportSchedulesToRoomFiles(
        const vector<Schedule>& schedules,
        const vector<ExamRoom>& allRooms,
        const string& baseDir = "data/room_students"
    ) {
        saveRoomStudentData(schedules, allRooms, baseDir);
    }
};
