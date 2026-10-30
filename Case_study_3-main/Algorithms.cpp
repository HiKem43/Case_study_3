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