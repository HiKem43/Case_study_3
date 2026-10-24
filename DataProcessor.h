#pragma once
#include <string>
#include <vector>
#include <map>
#include "models.h"
using namespace std;

// Lop xu ly du lieu dau vao cho bai toan xep lich thi
class DataProcessor {
private:
    // Danh sach du lieu doc tu file input.txt
    vector<Course> courses;
    vector<Student> students;
    vector<ExamRoom> rooms;
    vector<TimeSlot> timeSlots;

    // Do thi xung dot:
    // Moi ma mon hoc se lien ket voi cac mon co xung dot
    map<string, vector<string> > conflictGraph;

    // So luong du lieu duoc khai bao trong phan CONFIG
    int courseCount;
    int studentCount;
    int roomCount;
    int slotCount;

public:
    // Ham khoi tao: khoi tao cac so luong ban dau bang 0
    DataProcessor();

    // Doc va phan tich du lieu tu file
    bool loadFromFile(const string& filename);

    // Kiem tra tinh hop le cua du lieu da doc
    bool validateData();

    // Xay dung do thi xung dot giua cac mon hoc
    void buildConflictGraph();

    // Lay danh sach mon hoc
    vector<Course> getCourses() const;

    // Lay danh sach sinh vien
    vector<Student> getStudents() const;

    // Lay danh sach phong thi
    vector<ExamRoom> getRooms() const;

    // Lay danh sach khung gio thi
    vector<TimeSlot> getTimeSlots() const;

    // Lay do thi xung dot
    map<string, vector<string> > getConflictGraph() const;
};

