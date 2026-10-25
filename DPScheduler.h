#pragma once
#include <algorithm>
#include <climits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Models.h"
#include "ConflictGraph.h"

using namespace std;

// Dynamic Programming (Bitmask DP) cho bai toan xep lich thi.
// Muc tieu: xep tat ca mon vao so ca thi it nhat, khong vi pham xung dot,
// suc chua phong, thoi luong ca thi va viec mot phong khong the dung dong thoi.
namespace DPScheduler {

    static string makeDPId(int n) {
        string s = to_string(n);
        while (s.size() < 3) s = "0" + s;
        return "DP" + s;
    }

    static bool durationFits(const Course& course, const TimeSlot& slot) {
        int start = timeToMinutes(slot.getStartTime());
        int end = timeToMinutes(slot.getEndTime());
        return slot.isValid() && start >= 0 && end > start
            && end - start >= course.getDuration();
    }

    static int countBits(unsigned int x) {
        int count = 0;
        while (x) { count += (int)(x & 1U); x >>= 1U; }
        return count;
    }

    // Tra ve lich thi; failedCourses luu cac mon khong the xep.
    // courseStudentsMap: ma mon -> danh sach sinh vien dang ky mon do.
    inline vector<Schedule> runDPScheduler(
        const vector<Course>& courses,
        const vector<ExamRoom>& rooms,
        const vector<TimeSlot>& slots,
        const unordered_map<string, vector<Student>>& courseStudentsMap,
        vector<string>& failedCourses
    ) {
        vector<Schedule> result;
        failedCourses.clear();

        // Bitmask DP phu hop voi so mon nho (vi du 8 mon: 2^8 = 256 trang thai).
        const int n = (int)courses.size();
        if (n == 0) return result;
        if (n > 22 || rooms.empty() || slots.empty()) {
            for (const auto& c : courses) failedCourses.push_back(c.getCourseId());
            return result;
        }

        // Loai ma mon trung, mon khong hop le va mon khong the co phong du suc chua.
        vector<Course> cs;
        unordered_set<string> seen;
        for (const auto& c : courses) {
            if (!c.isValid() || seen.count(c.getCourseId())) continue;
            auto it = courseStudentsMap.find(c.getCourseId());
            int need = (it == courseStudentsMap.end()) ? 0 : (int)it->second.size();
            bool hasRoom = false;
            for (const auto& r : rooms)
                if (r.isValid() && r.getCapacity() >= need) hasRoom = true;
            if (!hasRoom) {
                failedCourses.push_back(c.getCourseId());
                continue;
            }
            seen.insert(c.getCourseId());
            cs.push_back(c);
        }

        const int m = (int)cs.size();
        if (m == 0) return result;
        if (m > 22) {
            for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
            return result;
        }

        ConflictGraph graph;
        graph.buildFromData(courseStudentsMap);
        for (const auto& c : cs) graph.addCourse(c.getCourseId());

        // Ma tran xung dot giua cac mon trong danh sach da chuan hoa.
        vector<vector<bool>> conflict(m, vector<bool>(m, false));
        for (int i = 0; i < m; ++i)
            for (int j = i + 1; j < m; ++j) {
                bool bad = graph.hasConflict(cs[i].getCourseId(), cs[j].getCourseId());
                conflict[i][j] = conflict[j][i] = bad;
            }

        const int total = 1 << m;
        const int INF = INT_MAX / 4;
        vector<int> dp(total, INF), parent(total, -1), chosenGroup(total, 0);
        dp[0] = 0;

        // Valid[T] = cac mon trong T co the thi cung mot ca va so mon <= so phong.
        vector<char> valid(total, false);
        valid[0] = true;
        for (int mask = 1; mask < total; ++mask) {
            if (countBits((unsigned int)mask) > (int)rooms.size()) continue;
            bool ok = true;
            for (int i = 0; i < m && ok; ++i) if (mask & (1 << i)) {
                for (int j = i + 1; j < m; ++j)
                    if ((mask & (1 << j)) && conflict[i][j]) { ok = false; break; }
            }
            valid[mask] = ok;
        }

        // Xet cac tap con T cua mask, chi cap nhat khi T la mot ca hop le.
        // De tranh lap lai cac phan hoach, T bat buoc chua bit dau tien cua mask.
        for (int mask = 1; mask < total; ++mask) {
            int firstBit = mask & -mask;
            for (int sub = mask; sub; sub = (sub - 1) & mask) {
                if (!(sub & firstBit) || !valid[sub]) continue;
                int rest = mask ^ sub;
                if (dp[rest] + 1 < dp[mask]) {
                    dp[mask] = dp[rest] + 1;
                    parent[mask] = rest;
                    chosenGroup[mask] = sub;
                }
            }
        }

        int full = total - 1;
        if (dp[full] >= INF || dp[full] > (int)slots.size()) {
            for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
            return result;
        }

        // Truy vet cac nhom mon theo cac ca thi.
        vector<int> groups;
        for (int mask = full; mask != 0; mask = parent[mask]) {
            if (parent[mask] < 0 || chosenGroup[mask] == 0) {
                for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
                return vector<Schedule>();
            }
            groups.push_back(chosenGroup[mask]);
        }
        reverse(groups.begin(), groups.end());

        // Sap xep phong tu nho den lon, de dung phong vua du truoc.
        vector<ExamRoom> sortedRooms = rooms;
        sort(sortedRooms.begin(), sortedRooms.end(), [](const ExamRoom& a, const ExamRoom& b) {
            if (a.getCapacity() != b.getCapacity()) return a.getCapacity() < b.getCapacity();
            return a.getRoomId() < b.getRoomId();
        });

        int counter = 1;
        for (int g = 0; g < (int)groups.size(); ++g) {
            int slotIndex = -1;
            for (int s = 0; s < (int)slots.size(); ++s) {
                bool allFit = true;
                for (int i = 0; i < m; ++i)
                    if ((groups[g] & (1 << i)) && !durationFits(cs[i], slots[s])) allFit = false;
                if (allFit) { slotIndex = s; break; }
            }
            if (slotIndex < 0) {
                for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
                return vector<Schedule>();
            }

            unordered_set<string> usedInThisSlot;
            for (int i = 0; i < m; ++i) {
                if (!(groups[g] & (1 << i))) continue;
                const Course& c = cs[i];
                auto itStudents = courseStudentsMap.find(c.getCourseId());
                int need = (itStudents == courseStudentsMap.end()) ? 0 : (int)itStudents->second.size();

                bool assigned = false;
                for (const auto& room : sortedRooms) {
                    if (!room.isValid() || room.getCapacity() < need) continue;
                    if (usedInThisSlot.count(room.getRoomId())) continue;

                    Schedule sch(makeDPId(counter++), c, room, slots[slotIndex]);
                    if (itStudents != courseStudentsMap.end()) {
                        for (const auto& st : itStudents->second) sch.addStudent(st);
                    }
                    if (!sch.isValid()) continue;
                    result.push_back(sch);
                    usedInThisSlot.insert(room.getRoomId());
                    assigned = true;
                    break;
                }
                if (!assigned) failedCourses.push_back(c.getCourseId());
            }
        }

        return result;
    }
}
