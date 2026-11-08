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

vector<Schedule> runGreedyScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    return GreedyScheduler::schedule(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
}

// Ban 5 tham so: giu nguyen cach goi cu, khong yeu cau luu kho rieng.
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

// Ban 6 tham so: giu nguyen chu ky con tro cua code cu,
// dong thoi cho phep BacktrackingScheduler ghi kho thi sinh theo phong.
vector<Schedule> runBacktrackingScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses,
    RoomCandidateMap* roomCandidatesByRoom
) {
    return BacktrackingScheduler::schedule(
        courses, rooms, slots, courseStudentsMap, failedCourses, roomCandidatesByRoom
    );
}

vector<Schedule> runDPScheduler(
    const vector<Course>& courses,
    const vector<ExamRoom>& rooms,
    const vector<TimeSlot>& slots,
    const unordered_map<string, vector<Student>>& courseStudentsMap,
    vector<string>& failedCourses
) {
    return DPScheduler::runDPScheduler(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
}
