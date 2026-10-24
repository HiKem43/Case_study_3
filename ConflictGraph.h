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
    // Thêm một môn học vào đồ thị (nếu môn đó đã có rồi thì giữ nguyên)
    void addCourse(const string& courseId) {
        string id = normalizeCode(courseId); // bỏ khoảng trống thừa
        // Nếu tìm không thấy mã môn này trong bảng adjList thì tạo một mãng rỗng
        if (!id.empty() && adjList.find(id) == adjList.end())
            adjList[id] = unordered_set<string>();
    }

    // Tạo cạnh xung đột 2 chiều giữa môn A và môn B
    // (Vì A đụng B thì B cũng đụng A)
    void addConflict(const string& courseA, const string& courseB) {
        string a = normalizeCode(courseA);
        string b = normalizeCode(courseB);
        if (a.empty() || b.empty() || a == b) return; // nếu a và b đều là toán thì cả 2 đều k thể xung đột với nhau vì toán thì làm sao xung đột đc với toán??????????????

        addCourse(a);
        addCourse(b);
        adjList[a].insert(b); 
        adjList[b].insert(a); 
    }

    // Dựng toàn bộ đồ thị từ dữ liệu: Mã môn -> Danh sách sinh viên học môn đó
    void buildFromData(const unordered_map<string, vector<Student>>& courseStudentsMap) {
        // Bước A: Đưa tất cả các môn có trong dữ liệu vào đồ thị trước
        // (Để những môn "cô lập" không trùng sinh viên với ai vẫn có mặt trong đồ thị)
        for (const auto& p : courseStudentsMap)
            addCourse(p.first); // p.first là Mã môn (VD: "C01")

        // Bước B: Gom nhóm lại xem MỖI SINH VIÊN đăng ký những môn nào
        // VD: studentCourses["ST13"] = {"C01", "C02", "C03"}
        unordered_map<string, vector<string>> studentCourses;
        for (const auto& p : courseStudentsMap) {
            string courseId = normalizeCode(p.first); // Mã môn
            for (const auto& st : p.second) {         // Duyệt từng sinh viên học môn đó
                studentCourses[st.getStudentId()].push_back(courseId);
            }
        }

        // Bước C: Với mỗi sinh viên, lấy từng cặp môn mà sinh viên đó học chung
        // để đánh dấu 2 môn đó xung đột với nhau.
        // VD: ST13 học {"C01", "C02", "C03"} -> Tạo 3 cặp xung đột: (C01-C02), (C01-C03), (C02-C03)
        for (const auto& p : studentCourses) {
            const vector<string>& list = p.second; // Danh sách các môn của 1 sinh viên
            for (size_t i = 0; i < list.size(); ++i)
                for (size_t j = i + 1; j < list.size(); ++j) // j = i + 1 để không xét trùng cặp
                    addConflict(list[i], list[j]);
        }
    }

    // Hàm phụ hỗ trợ: Dựng đồ thị trực tiếp từ danh sách vector<Student>
    void buildFromStudents(const vector<Student>& students) {
        for (const auto& st : students) {
            vector<string> list = st.getRegisteredCourses();
            for (size_t i = 0; i < list.size(); ++i) {
                addCourse(list[i]);
                for (size_t j = i + 1; j < list.size(); ++j)
                    addConflict(list[i], list[j]);
            }
        }
    }

    // Kiểm tra nhanh 2 môn A và B có xung đột với nhau không (Trả về true/false)
    bool hasConflict(const string& courseA, const string& courseB) const {
        auto it = adjList.find(normalizeCode(courseA));
        if (it == adjList.end()) return false; // Không tìm thấy môn A
        return it->second.count(normalizeCode(courseB)) > 0; // Xem B có nằm trong túi của A không
    }

    // Lấy "Bậc" của một môn = Tổng số môn bị xung đột trực tiếp với môn này
    // VD: C01 đụng 6 môn khác -> getDegree("C01") trả về 6.
    int getDegree(const string& courseId) const {
        auto it = adjList.find(normalizeCode(courseId));
        return it == adjList.end() ? 0 : (int)it->second.size();
    }

    // Lấy ra danh sách tất cả các mã môn xung đột với môn courseId
    const unordered_set<string>& getNeighbors(const string& courseId) const {
        static const unordered_set<string> emptySet; // Tập rỗng dự phòng khi môn không tồn tại
        auto it = adjList.find(normalizeCode(courseId));
        return it == adjList.end() ? emptySet : it->second;
    }
};

