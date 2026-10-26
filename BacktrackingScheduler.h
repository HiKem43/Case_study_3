#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
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
    vector<int> currentSlot; // currentSlot[i]: chỉ số ca thi của môn i (-1 nếu không xếp được)
    vector<int> currentRoom; // currentRoom[i]: chỉ số phòng thi của môn i (-1 nếu không xếp được)

    // Lưu nghiệm tốt nhất (Best Solution) phục vụ Cắt tỉa (Pruning)
    vector<int> bestSlot;
    vector<int> bestRoom;
    int bestAssignedCount; // Kỷ lục: Số môn xếp lịch thành công nhiều nhất (ưu tiên 1 - càng lớn càng tốt)
    int bestMaxSlotIndex;  // Kỷ lục: Chỉ số ca thi lớn nhất phải dùng (ưu tiên 2 - càng nhỏ càng tốt)

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
    // Trả về chỉ số phòng hợp lệ (tối ưu sức chứa nhất) cho môn courseIdx tại ca slotIdx, hoặc -1 nếu không hợp lệ
    int findValidRoomForSlot(int courseIdx, int slotIdx) const {
        const Course& course = validCourses[courseIdx];
        const TimeSlot& slot = slots[slotIdx];

        // Kiểm tra tính hợp lệ của ca thi và thời lượng làm bài
        if (!slot.isValid() || !isSlotDurationEnough(course, slot)) {
            return -1;
        }

        string cId = course.getCourseId();

        // Kiểm tra xung đột sinh viên với các môn đã xếp trước đó (tính cả các ca trùng giờ overlaps)
        for (int i = 0; i < courseIdx; ++i) {
            int prevSlot = currentSlot[i];
            if (prevSlot != -1 && slots[prevSlot].overlaps(slot)) {
                if (graph.hasConflict(cId, validCourses[i].getCourseId())) {
                    return -1; // Bị trùng lịch sinh viên
                }
            }
        }

        int need = getStudentCount(cId);

        // Tìm phòng trống đủ sức chứa (sortedRooms đã sắp xếp tăng dần theo capacity)
        for (int r = 0; r < numRooms; ++r) {
            if (!sortedRooms[r].isValid() || sortedRooms[r].getCapacity() < need) {
                continue;
            }

            // Kiểm tra phòng r có bị môn nào trước đó chiếm trong ca trùng giờ không
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
                return r; // Chọn ngay phòng vừa khít nhất (Cắt tỉa các phòng lớn hơn không cần thiết)
            }
        }

        return -1; // Không có phòng trống thỏa mãn trong ca này
    }

    // 2. HÀM ĐỆ QUY, QUAY LUI VÀ CẮT TỈA
    void backtrack(int courseIdx, int assignedCount, int maxSlotUsed) {
        int remaining = numCourses - courseIdx;

        // [CẮT TỈA 1 - PRUNING]: Nếu dù xếp thành công toàn bộ các môn còn lại
        // mà vẫn ít hơn kỷ lục số môn đã xếp được tốt nhất -> Cắt nhánh
        if (assignedCount + remaining < bestAssignedCount) {
            return;
        }

        // [CẮT TỈA 2 - PRUNING]: Nếu tối đa chỉ có thể bằng số môn kỷ lục,
        // nhưng số ca thi hiện tại đã >= kỷ lục ca thi tốt nhất -> Cắt nhánh
        if (assignedCount + remaining == bestAssignedCount && maxSlotUsed >= bestMaxSlotIndex) {
            return;
        }

        // [ĐIỀU KIỆN DỪNG]: Đã duyệt xong tất cả các môn học
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

        // Duyệt thử từng ca thi cho môn học hiện tại (courseIdx)
        for (int s = 0; s < numSlots; ++s) {
            int nextMaxSlot = max(maxSlotUsed, s + 1);

            // [CẮT TỈA SỚM TRONG VÒNG LẶP]: Nếu đã đạt tối đa số môn và ca s làm vượt kỷ lục ca thi -> Bỏ qua
            if (assignedCount + remaining == bestAssignedCount && nextMaxSlot >= bestMaxSlotIndex) {
                continue;
            }

            int r = findValidRoomForSlot(courseIdx, s);
            if (r != -1) {
                // --- THỬ (DO) ---
                currentSlot[courseIdx] = s;
                currentRoom[courseIdx] = r;

                // --- ĐỆ QUY (RECURSE) ---
                backtrack(courseIdx + 1, assignedCount + 1, nextMaxSlot);

                // --- QUAY LUI (BACKTRACK / UNDO) ---
                currentSlot[courseIdx] = -1;
                currentRoom[courseIdx] = -1;
            }
        }

        // Nhánh dự phòng: Trong trường hợp môn học không thể xếp vào bất kỳ ca nào
        // (cho phép bỏ qua môn này để xếp tiếp các môn còn lại giống cơ chế failedCourses của Greedy)
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

        // Dựng đồ thị xung đột từ dữ liệu
        graph.buildFromData(courseStudentsMap);
        for (const auto& c : courses) {
            graph.addCourse(c.getCourseId());
        }

        // Lọc các môn không hợp lệ hoặc bị trùng mã (đồng bộ với GreedyScheduler)
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

        // Sắp xếp môn học theo bậc xung đột giảm dần (Heuristic giúp cắt tỉa sớm hiệu quả hơn)
        sort(validCourses.begin(), validCourses.end(), [&](const Course& a, const Course& b) {
            int da = graph.getDegree(a.getCourseId());
            int db = graph.getDegree(b.getCourseId());
            if (da != db) return da > db;

            int sa = getStudentCount(a.getCourseId());
            int sb = getStudentCount(b.getCourseId());
            if (sa != sb) return sa > sb;

            return a.getCourseId() < b.getCourseId();
            });

        // Sắp xếp phòng thi tăng dần theo sức chứa
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

    // Hàm tĩnh gọi thực thi thuật toán Quay lui & Cắt tỉa (đồng bộ chữ ký với GreedyScheduler::schedule)
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

        return result;
    }
};