#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Models.h"

using namespace std;

class ConflictGraph {
private:
    // Bảng lưu danh sách xung đột của từng môn.
    // Chìa khóa (string): Mã môn học (VD: "C01")
    // Giá trị (unordered_set<string>): Túi chứa các môn xung đột với nó (VD: {"C02", "C03"})
    // Dùng unordered_set để tự động lọc trùng (nhét "C02" nhiều lần vẫn chỉ lưu 1 lần).
    unordered_map<string, unordered_set<string>> adjList;

public:
    // Thêm một môn học vào đồ thị
    bool addCourse(const string& courseId) {
        if (!isValidCode(courseId)) return false;
        if (adjList.find(courseId) == adjList.end()) {
            adjList[courseId] = unordered_set<string>();
        }
        return true;
    }

    // Tạo cạnh xung đột 2 chiều giữa môn A và môn B
    // (Vì A đụng B thì B cũng đụng A)
    bool addConflict(const string& courseA, const string& courseB) {
        if (!isValidCode(courseA) || !isValidCode(courseB) || courseA == courseB) {
            return false;
        }
        addCourse(courseA);
        addCourse(courseB);
        adjList[courseA].insert(courseB);
        adjList[courseB].insert(courseA);
        return true;
    }

    // Dựng mới toàn bộ đồ thị từ bảng ánh xạ: Mã môn -> Danh sách sinh viên
    void buildFromData(const unordered_map<string, vector<Student>>& courseStudentsMap) {
        adjList.clear(); // Xóa dữ liệu cũ trước khi dựng đồ thị mới

        for (const auto& p : courseStudentsMap) {
            addCourse(p.first);
        }

        unordered_map<string, vector<string>> studentCourses;
        for (const auto& p : courseStudentsMap) {
            if (!isValidCode(p.first)) continue;
            for (const auto& st : p.second) {
                if (isValidCode(st.getStudentId())) {
                    studentCourses[st.getStudentId()].push_back(p.first);
                }
            }
        }

        for (const auto& p : studentCourses) {
            const vector<string>& list = p.second;
            for (size_t i = 0; i < list.size(); ++i) {
                for (size_t j = i + 1; j < list.size(); ++j) {
                    addConflict(list[i], list[j]);
                }
            }
        }
    }

    // Dựng mới toàn bộ đồ thị trực tiếp từ danh sách vector<Student>
    void buildFromStudents(const vector<Student>& students) {
        adjList.clear(); // Xóa dữ liệu cũ trước khi dựng đồ thị mới

        for (const auto& st : students) {
            vector<string> list = st.getRegisteredCourses();
            for (size_t i = 0; i < list.size(); ++i) {
                addCourse(list[i]);
                for (size_t j = i + 1; j < list.size(); ++j) {
                    addConflict(list[i], list[j]);
                }
            }
        }
    }

    // Kiểm tra nhanh 2 môn A và B có xung đột với nhau không (Trả về true/false)
    bool hasConflict(const string& courseA, const string& courseB) const {
        if (!isValidCode(courseA) || !isValidCode(courseB)) return false;
        auto it = adjList.find(courseA);
        if (it == adjList.end()) return false; // Không tìm thấy môn A
        return it->second.count(courseB) > 0;  // Xem B có nằm trong túi của A không
    }

    // Lấy "Bậc" của một môn = Tổng số môn bị xung đột trực tiếp với môn này
    // VD: C01 đụng 6 môn khác -> getDegree("C01") trả về 6.
    int getDegree(const string& courseId) const {
        if (!isValidCode(courseId)) return 0;
        auto it = adjList.find(courseId);
        return it == adjList.end() ? 0 : (int)it->second.size();
    }

    // Lấy tập hợp các mã môn xung đột với môn này
    const unordered_set<string>& getNeighbors(const string& courseId) const {
        static const unordered_set<string> emptySet;
        if (!isValidCode(courseId)) return emptySet;
        auto it = adjList.find(courseId);
        return it == adjList.end() ? emptySet : it->second;
    }
};