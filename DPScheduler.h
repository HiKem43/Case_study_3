#pragma once
#include <algorithm>
#include <climits>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Models.h"
#include "ConflictGraph.h"

using namespace std;

// Dynamic Programming (Bitmask DP) cho bai toan xep lich thi.
// Muc tieu: xep tat ca mon vao so ca thi it nhat, dong thoi thoa man
// xung dot, thoi luong, suc chua phong, phong khong bi dung trung va ca thi khong chong gio.
namespace DPScheduler {

    static string makeDPId(int n) {
        string s = to_string(n);
        while (s.size() < 3) s = "0" + s;
        return "DP" + s;
    }

    static bool durationFits(const Course& course, const TimeSlot& slot) {
        int start = timeToMinutes(slot.getStartTime());
        int end = timeToMinutes(slot.getEndTime());
        int duration = course.getDuration();
        return slot.isValid() && start >= 0 && end > start
            && duration > 0 && end - start >= duration;
    }

    static bool slotsOverlap(const TimeSlot& a, const TimeSlot& b) {
        int aStart = timeToMinutes(a.getStartTime());
        int aEnd = timeToMinutes(a.getEndTime());
        int bStart = timeToMinutes(b.getStartTime());
        int bEnd = timeToMinutes(b.getEndTime());
        if (aStart < 0 || aEnd <= aStart || bStart < 0 || bEnd <= bStart)
            return true; // Slot khong doc duoc thoi gian: xem nhu khong an toan.
        return aStart < bEnd && bStart < aEnd;
    }

    static int countBits(unsigned int x) {
        int count = 0;
        while (x) { count += (int)(x & 1U); x >>= 1U; }
        return count;
    }

    // Tim cach gan moi mon trong mot nhom vao mot phong rieng biet.
    // Thu mon co si so lon truoc de tranh chon phong khong phu hop qua som.
    static bool assignRoomsForGroup(
        int groupMask,
        const vector<Course>& courses,
        const vector<ExamRoom>& rooms,
        const unordered_map<string, vector<Student>>& courseStudentsMap,
        vector<int>& roomForCourse
    ) {
        vector<int> courseOrder;
        for (int i = 0; i < (int)courses.size(); ++i)
            if (groupMask & (1 << i)) courseOrder.push_back(i);

        sort(courseOrder.begin(), courseOrder.end(), [&](int a, int b) {
            auto ia = courseStudentsMap.find(courses[a].getCourseId());
            auto ib = courseStudentsMap.find(courses[b].getCourseId());
            int na = (ia == courseStudentsMap.end()) ? 0 : (int)ia->second.size();
            int nb = (ib == courseStudentsMap.end()) ? 0 : (int)ib->second.size();
            return na > nb;
        });

        vector<bool> used(rooms.size(), false);
        function<bool(int)> dfs = [&](int pos) {
            if (pos == (int)courseOrder.size()) return true;
            int ci = courseOrder[pos];
            auto it = courseStudentsMap.find(courses[ci].getCourseId());
            int need = (it == courseStudentsMap.end()) ? 0 : (int)it->second.size();

            vector<int> candidates;
            for (int r = 0; r < (int)rooms.size(); ++r)
                if (!used[r] && rooms[r].isValid() && rooms[r].getCapacity() >= need)
                    candidates.push_back(r);
            sort(candidates.begin(), candidates.end(), [&](int a, int b) {
                if (rooms[a].getCapacity() != rooms[b].getCapacity())
                    return rooms[a].getCapacity() < rooms[b].getCapacity();
                return rooms[a].getRoomId() < rooms[b].getRoomId();
            });

            for (int ri : candidates) {
                used[ri] = true;
                roomForCourse[ci] = ri;
                if (dfs(pos + 1)) return true;
                roomForCourse[ci] = -1;
                used[ri] = false;
            }
            return false;
        };
        return dfs(0);
    }

    // Gan cac nhom mon vao cac ca thi khac nhau va khong chong gio.
    // Neu mot cach gan that bai, thu ca thi khac thay vi dung ngay.
    static bool assignSlotsToGroups(
        const vector<int>& groups,
        const vector<Course>& courses,
        const vector<TimeSlot>& slots,
        vector<int>& slotForGroup
    ) {
        const int gCount = (int)groups.size();
        vector<vector<int>> candidates(gCount);
        for (int g = 0; g < gCount; ++g) {
            for (int s = 0; s < (int)slots.size(); ++s) {
                bool fits = true;
                for (int i = 0; i < (int)courses.size(); ++i)
                    if ((groups[g] & (1 << i)) && !durationFits(courses[i], slots[s])) {
                        fits = false;
                        break;
                    }
                if (fits) candidates[g].push_back(s);
            }
            if (candidates[g].empty()) return false;
        }

        // Sap xep nhom it lua chon nhat truoc (heuristic), van thu tat ca phuong an.
        vector<int> order(gCount);
        for (int i = 0; i < gCount; ++i) order[i] = i;
        sort(order.begin(), order.end(), [&](int a, int b) {
            return candidates[a].size() < candidates[b].size();
        });

        vector<int> selected;
        function<bool(int)> dfs = [&](int pos) {
            if (pos == gCount) return true;
            int gi = order[pos];
            for (int si : candidates[gi]) {
                bool conflictTime = false;
                for (int usedSlot : selected)
                    if (slotsOverlap(slots[si], slots[usedSlot])) {
                        conflictTime = true;
                        break;
                    }
                if (conflictTime) continue;

                slotForGroup[gi] = si;
                selected.push_back(si);
                if (dfs(pos + 1)) return true;
                selected.pop_back();
                slotForGroup[gi] = -1;
            }
            return false;
        };
        return dfs(0);
    }

    // Tra ve lich thi. Neu khong the xep day du, result rong va failedCourses
    // chua cac ma mon chua duoc xep (che do all-or-nothing).
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
        if (courses.empty()) return result;

        // Bitmask su dung int nen gioi han 22 mon de tranh tran bit va qua tai bo nho.
        if (courses.size() > 22 || rooms.empty() || slots.empty()) {
            for (const auto& c : courses) failedCourses.push_back(c.getCourseId());
            return result;
        }

        // Chuan hoa du lieu: mon khong hop le, thoi luong <= 0 va ma mon trung deu duoc bao loi.
        vector<Course> cs;
        unordered_set<string> seen;
        for (const auto& c : courses) {
            const string id = c.getCourseId();
            if (!c.isValid() || c.getDuration() <= 0 || id.empty() || seen.count(id)) {
                failedCourses.push_back(id);
                continue;
            }
            seen.insert(id);
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

        vector<vector<bool>> conflict(m, vector<bool>(m, false));
        for (int i = 0; i < m; ++i)
            for (int j = i + 1; j < m; ++j) {
                bool bad = graph.hasConflict(cs[i].getCourseId(), cs[j].getCourseId());
                conflict[i][j] = conflict[j][i] = bad;
            }

        // Kiem tra kha nang co it nhat mot phong va it nhat mot ca phu hop cho tung mon.
        vector<bool> individuallyFeasible(m, false);
        for (int i = 0; i < m; ++i) {
            auto it = courseStudentsMap.find(cs[i].getCourseId());
            int need = (it == courseStudentsMap.end()) ? 0 : (int)it->second.size();
            bool roomOK = false, slotOK = false;
            for (const auto& r : rooms)
                if (r.isValid() && r.getCapacity() >= need) roomOK = true;
            for (const auto& s : slots)
                if (durationFits(cs[i], s)) slotOK = true;
            individuallyFeasible[i] = roomOK && slotOK;
            if (!individuallyFeasible[i]) failedCourses.push_back(cs[i].getCourseId());
        }
        if (!failedCourses.empty()) return result;

        const int total = 1 << m;
        const int INF = INT_MAX / 4;
        vector<int> dp(total, INF);
        dp[0] = 0;

        // Valid[T]: khong co xung dot va co the phan vao cac phong khac nhau.
        vector<char> valid(total, false);
        valid[0] = true;
        vector<vector<int>> roomAssignment(total);
        for (int mask = 1; mask < total; ++mask) {
            if (countBits((unsigned int)mask) > (int)rooms.size()) continue;
            bool noConflict = true;
            for (int i = 0; i < m && noConflict; ++i) if (mask & (1 << i))
                for (int j = i + 1; j < m; ++j)
                    if ((mask & (1 << j)) && conflict[i][j]) {
                        noConflict = false;
                        break;
                    }
            if (!noConflict) continue;

            vector<int> assignment(m, -1);
            if (assignRoomsForGroup(mask, cs, rooms, courseStudentsMap, assignment)) {
                valid[mask] = true;
                roomAssignment[mask] = assignment;
            }
        }

        // Bitmask DP: dp[mask] la so nhom/ca it nhat de xep cac mon trong mask.
        for (int mask = 1; mask < total; ++mask) {
            int firstBit = mask & -mask;
            for (int sub = mask; sub; sub = (sub - 1) & mask) {
                if (!(sub & firstBit) || !valid[sub]) continue;
                int rest = mask ^ sub;
                if (dp[rest] != INF && dp[rest] + 1 < dp[mask])
                    dp[mask] = dp[rest] + 1;
            }
        }

        const int full = total - 1;
        if (dp[full] >= INF || dp[full] > (int)slots.size()) {
            for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
            return result;
        }

        // Tim phuong an phan nhom co so ca nho nhat va gan duoc vao cac ca thuc te.
        // Neu phuong an toi uu theo DP khong the gan do trung gio, thu phuong an
        // co nhieu ca hon; khong ket luan that bai qua som.
        vector<int> groups;
        vector<int> slotForGroup;
        bool foundPlan = false;
        for (int target = dp[full]; target <= m && target <= (int)slots.size(); ++target) {
            groups.clear();
            function<bool(int)> buildPartition = [&](int remaining) {
                if (remaining == 0) {
                    if ((int)groups.size() != target) return false;
                    vector<int> candidateSlots(groups.size(), -1);
                    if (assignSlotsToGroups(groups, cs, slots, candidateSlots)) {
                        slotForGroup = candidateSlots;
                        return true;
                    }
                    return false;
                }
                if ((int)groups.size() >= target) return false;
                int firstBit = remaining & -remaining;
                for (int sub = remaining; sub; sub = (sub - 1) & remaining) {
                    if (!(sub & firstBit) || !valid[sub]) continue;
                    int rest = remaining ^ sub;
                    int groupsStillNeeded = target - (int)groups.size() - 1;
                    if (rest != 0 && dp[rest] > groupsStillNeeded) continue;
                    if (rest == 0 && groupsStillNeeded != 0) continue;
                    groups.push_back(sub);
                    if (buildPartition(rest)) return true;
                    groups.pop_back();
                }
                return false;
            };
            if (buildPartition(full)) {
                foundPlan = true;
                break;
            }
        }
        if (!foundPlan) {
            for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
            return result;
        }

        // Chi tao Schedule sau khi da tim duoc phuong an hop le cho toan bo mon.
        int counter = 1;
        for (int g = 0; g < (int)groups.size(); ++g) {
            const int mask = groups[g];
            const int slotIndex = slotForGroup[g];
            for (int i = 0; i < m; ++i) {
                if (!(mask & (1 << i))) continue;
                int roomIndex = roomAssignment[mask][i];
                if (roomIndex < 0 || roomIndex >= (int)rooms.size()) {
                    result.clear();
                    for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
                    return result;
                }

                Schedule sch(makeDPId(counter++), cs[i], rooms[roomIndex], slots[slotIndex]);
                auto it = courseStudentsMap.find(cs[i].getCourseId());
                if (it != courseStudentsMap.end())
                    for (const auto& st : it->second) sch.addStudent(st);
                if (!sch.isValid()) {
                    result.clear();
                    for (const auto& c : cs) failedCourses.push_back(c.getCourseId());
                    return result;
                }
                result.push_back(sch);
            }
        }

        return result;
    }
}
