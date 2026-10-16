#pragma once

#include <string>
#include <vector>

using namespace std;

// CLASS COURSE
// Luu thong tin mon hoc
class Course {
private:
    string courseId;    // Ma mon hoc
    string courseName;  // Ten mon hoc
    int credits;        // So tin chi

public:
    // Constructor mac dinh
    Course() : credits(0) {}

    // Constructor co tham so
    Course(string id, string name, int cr)
        : courseId(id), courseName(name), credits(cr) {}

    // Cac ham lay thong tin mon hoc
    string getCourseId() const {
        return courseId;
    }

    string getCourseName() const {
        return courseName;
    }

    int getCredits() const {
        return credits;
    }
};

//CLASS STUDENT
// Luu thong tin sinh vien
class Student {
private:
    string studentId;   // Ma sinh vien
    string fullName;    // Ho ten sinh vien

public:
    Student() {}

    Student(string id, string name)
        : studentId(id), fullName(name) {}

    string getStudentId() const {
        return studentId;
    }

    string getFullName() const {
        return fullName;
    }
};


// CLASS EXAMROOM 
// Luu thong tin phong thi
class ExamRoom {
private:
    string roomId;  // Ma phong thi
    int capacity;   // So sinh vien toi da

public:
    ExamRoom() : capacity(0) {}

    ExamRoom(string id, int cap)
        : roomId(id), capacity(cap) {}

    string getRoomId() const {
        return roomId;
    }

    int getCapacity() const {
        return capacity;
    }
};


// CLASS TIMESLOT 
// Luu ngay va gio thi
class TimeSlot {
private:
    string examDate;    // Ngay thi: YYYY-MM-DD
    string startTime;   // Gio bat dau: HH:MM
    string endTime;     // Gio ket thuc: HH:MM

public:
    TimeSlot() {}

    TimeSlot(string date, string start, string end)
        : examDate(date), startTime(start), endTime(end) {}

    string getExamDate() const {
        return examDate;
    }

    string getStartTime() const {
        return startTime;
    }

    string getEndTime() const {
        return endTime;
    }
};


// CLASS SCHEDULE
// Luu thong tin mot lich thi
class Schedule {
private:
    string scheduleId;      // Ma lich thi
    Course course;          // Mon thi
    ExamRoom room;          // Phong thi
    TimeSlot timeSlot;      // Khung gio thi
    vector<Student> students; // Danh sach sinh vien thi

public:
    Schedule() {}

    // Tao lich thi voi mon, phong va thoi gian
    Schedule(string id, Course c, ExamRoom r, TimeSlot t)
        : scheduleId(id), course(c), room(r), timeSlot(t) {}

    string getScheduleId() const {
        return scheduleId;
    }

    // Them sinh vien vao lich thi
    bool addStudent(const Student& student) {
        // 1. Kiem tra sinh vien da co trong danh sach chua
        for (const Student& s : students) {
            if (s.getStudentId() == student.getStudentId()) {
                return false; // Khong them trung
            }
        }

        // 2. Kiem tra phong thi con cho khong
        if (students.size() >= static_cast<size_t>(room.getCapacity())) {
            return false; // Phong da day
        }

        // 3. Neu hop le, them sinh vien vao danh sach
        students.push_back(student);

        return true; // Them thanh cong
    }

    // Lay danh sach sinh vien trong lich thi
    const vector<Student>& getStudents() const {
        return students;
    }
};
