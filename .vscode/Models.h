
#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <iostream>

using namespace std;
class Course {
private:
    string courseId;
    string courseName;
    int credits;

public:
    Course() : credits(0) {}

    Course(string id, string name, int cr)
        : courseId(id), courseName(name), credits(cr) {}

    string getCourseId() const { return courseId; }
    string getCourseName() const { return courseName; }
    int getCredits() const { return credits; }

    void display() const {
        cout << "Ma mon: " << courseId
             << ", Ten mon: " << courseName
             << ", Tin chi: " << credits << endl;
    }
};

class Student {
private:
    string studentId;
    string fullName;

public:
    Student() {}

    Student(string id, string name)
        : studentId(id), fullName(name) {}

    string getStudentId() const { return studentId; }
    string getFullName() const { return fullName; }

    void display() const {
        cout << "Ma SV: " << studentId
             << ", Ho ten: " << fullName << endl;
    }
};

class ExamRoom {
private:
    string roomId;
    int capacity;

public:
    ExamRoom() : capacity(0) {}

    ExamRoom(string id, int cap)
        : roomId(id), capacity(cap) {}

    string getRoomId() const { return roomId; }
    int getCapacity() const { return capacity; }

    void display() const {
        cout << "Ma phong: " << roomId
             << ", Suc chua: " << capacity << endl;
    }
};

class TimeSlot {
private:
    string examDate;     // YYYY-MM-DD
    string startTime;    // HH:MM
    string endTime;      // HH:MM

public:
    TimeSlot() {}

    TimeSlot(string date, string start, string end)
        : examDate(date), startTime(start), endTime(end) {}

    string getExamDate() const { return examDate; }
    string getStartTime() const { return startTime; }
    string getEndTime() const { return endTime; }

    void display() const {
        cout << "Ngay thi: " << examDate
             << ", Tu " << startTime
             << " den " << endTime << endl;
    }
};

class Schedule {
private:
    string scheduleId;
    Course course;
    ExamRoom room;
    TimeSlot timeSlot;
    vector<Student> students;

public:
    Schedule() {}

    Schedule(string id, Course c, ExamRoom r, TimeSlot t)
        : scheduleId(id), course(c), room(r), timeSlot(t) {}

    string getScheduleId() const {
        return scheduleId;
    }

    void addStudent(const Student& student) {
        // Không thêm sinh viên trùng mã
        for (const Student& s : students) {
            if (s.getStudentId() == student.getStudentId()) {
                return;
            }
        }

        // kiem tra suc chua phong
        if (students.size() >= static_cast<size_t>(room.getCapacity())) {
            cout << "Phong thi da day!" << endl;
            return;
        }

        students.push_back(student);
    }

    const vector<Student>& getStudents() const {
        return students;
    }

    void display() const {
        cout << "\n=== LICH THI ===" << endl;
        cout << "Ma lich: " << scheduleId << endl;

        course.display();
        room.display();
        timeSlot.display();

        cout << "Danh sach sinh vien:" << endl;
        for (const Student& s : students) {
            s.display();
        }

        cout << "Tong so sinh vien: "
             << students.size() << endl;
    }
};

#endif 
