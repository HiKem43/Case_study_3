#pragma once
#include <string>
#include <vector>
#include <cctype>

using namespace std;

// ==================== HAM KIEM TRA CHUNG ====================

// Kiem tra chuoi khong rong
bool isNotEmpty(const string& s) {
    for (int i = 0; i < (int)s.length(); i++) {
        if (!isspace((unsigned char)s[i])) {
            return true;
        }
    }
    return false;
}

// Kiem tra ma chi gom chu, so, dau gach ngang va gach duoi
bool isValidCode(const string& code) {
    if (!isNotEmpty(code)) {
        return false;
    }

    for (int i = 0; i < (int)code.length(); i++) {
        char c = code[i];

        if (!isalnum((unsigned char)c) && c != '-' && c != '_') {
            return false;
        }
    }

    return true;
}

// Kiem tra ngay theo dinh dang YYYY-MM-DD
bool isValidDate(const string& date) {
    if (date.length() != 10 ||
        date[4] != '-' || date[7] != '-') {
        return false;
    }

    for (int i = 0; i < 10; i++) {
        if (i != 4 && i != 7 &&
            !isdigit((unsigned char)date[i])) {
            return false;
        }
    }

    int year = (date[0] - '0') * 1000
             + (date[1] - '0') * 100
             + (date[2] - '0') * 10
             + (date[3] - '0');

    int month = (date[5] - '0') * 10
              + (date[6] - '0');

    int day = (date[8] - '0') * 10
            + (date[9] - '0');

    if (year < 1900 || month < 1 || month > 12) {
        return false;
    }

    int daysInMonth[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    // Kiem tra nam nhuan
    bool leapYear = (year % 400 == 0) ||
                    (year % 4 == 0 && year % 100 != 0);

    if (leapYear) {
        daysInMonth[1] = 29;
    }

    if (day < 1 || day > daysInMonth[month - 1]) {
        return false;
    }

    return true;
}

// Chuyen gio HH:MM thanh so phut
// Tra ve -1 neu gio khong hop le
int timeToMinutes(const string& time) {
    if (time.length() != 5 || time[2] != ':') {
        return -1;
    }

    if (!isdigit((unsigned char)time[0]) ||
        !isdigit((unsigned char)time[1]) ||
        !isdigit((unsigned char)time[3]) ||
        !isdigit((unsigned char)time[4])) {
        return -1;
    }

    int hour = (time[0] - '0') * 10
             + (time[1] - '0');

    int minute = (time[3] - '0') * 10
               + (time[4] - '0');

    if (hour < 0 || hour > 23 ||
        minute < 0 || minute > 59) {
        return -1;
    }

    return hour * 60 + minute;
}

// ==================== CLASS COURSE ====================

class Course {
private:
    string courseId;
    string courseName;
    int duration;

public:
    Course() {
        courseId = "";
        courseName = "";
        duration = 0;
    }

    Course(string id, string name, int dur) {
        courseId = id;
        courseName = name;
        duration = dur;
    }

    bool isValid() const {
        return isValidCode(courseId) &&
               isNotEmpty(courseName) &&
               duration > 0;
    }

    string getCourseId() const {
        return courseId;
    }

    string getCourseName() const {
        return courseName;
    }

    int getDuration() const {
        return duration;
    }
};

// ==================== CLASS STUDENT ====================

class Student {
private:
    string studentId;
    string fullName;
    string dateOfBirth;
    string gender;
    vector<string> registeredCourses;

public:
    Student() {
        studentId = "";
        fullName = "";
        dateOfBirth = "";
        gender = "";
    }

    Student(string id, string name, string dob,
            string g, vector<string> courses) {
        studentId = id;
        fullName = name;
        dateOfBirth = dob;
        gender = g;
        registeredCourses = courses;
    }

    bool isValid() const {
        if (!isValidCode(studentId) ||
            !isNotEmpty(fullName) ||
            !isValidDate(dateOfBirth)) {
            return false;
        }

        if (gender != "Nam" &&
            gender != "Nu" &&
            gender != "Khac") {
            return false;
        }

        // Kiem tra ma mon hoc dang ky
        for (int i = 0; i < (int)registeredCourses.size(); i++) {
            if (!isValidCode(registeredCourses[i])) {
                return false;
            }

            // Khong cho phep dang ky trung mon
            for (int j = i + 1;
                 j < (int)registeredCourses.size(); j++) {
                if (registeredCourses[i] == registeredCourses[j]) {
                    return false;
                }
            }
        }

        return true;
    }

    string getStudentId() const {
        return studentId;
    }

    string getFullName() const {
        return fullName;
    }

    string getDateOfBirth() const {
        return dateOfBirth;
    }

    string getGender() const {
        return gender;
    }

    vector<string> getRegisteredCourses() const {
        return registeredCourses;
    }

    bool hasRegisteredCourse(const string& courseId) const {
        for (int i = 0; i < (int)registeredCourses.size(); i++) {
            if (registeredCourses[i] == courseId) {
                return true;
            }
        }

        return false;
    }
};

// ==================== CLASS EXAMROOM ====================

class ExamRoom {
private:
    string roomId;
    int capacity;

public:
    ExamRoom() {
        roomId = "";
        capacity = 0;
    }

    ExamRoom(string id, int cap) {
        roomId = id;
        capacity = cap;
    }

    bool isValid() const {
        return isValidCode(roomId) && capacity > 0;
    }

    string getRoomId() const {
        return roomId;
    }

    int getCapacity() const {
        return capacity;
    }
};

// ==================== CLASS TIMESLOT ====================

class TimeSlot {
private:
    string examDate;
    string startTime;
    string endTime;

public:
    TimeSlot() {
        examDate = "";
        startTime = "";
        endTime = "";
    }

    TimeSlot(string date, string start, string end) {
        examDate = date;
        startTime = start;
        endTime = end;
    }

    bool isValid() const {
        int start = timeToMinutes(startTime);
        int end = timeToMinutes(endTime);

        return isValidDate(examDate) &&
               start >= 0 &&
               end >= 0 &&
               end > start;
    }

    string getExamDate() const {
        return examDate;
    }

    string getStartTime() const {
        return startTime;
    }

    string getEndTime() const {
        return endTime;
    }

    // Kiem tra hai khung gio co trung nhau khong
    bool overlaps(const TimeSlot& other) const {
        if (!isValid() || !other.isValid()) {
            return false;
        }
class TimeSlot {
private:
    string timeSlotId;  // Ma ca thi
    string examDate;    // Ngay thi
    string startTime;   // Gio bat dau
    string endTime;     // Gio ket thuc

public:
    // Ham khoi tao mac dinh
    TimeSlot() {
        timeSlotId = "";
        examDate = "";
        startTime = "";
        endTime = "";
    }

    // Ham khoi tao co tham so
    TimeSlot(string id, string date, string start, string end) {
        timeSlotId = id;
        examDate = date;
        startTime = start;
        endTime = end;
    }

    // Kiem tra tinh hop le cua ca thi
    bool isValid() const {
        int start = timeToMinutes(startTime);
        int end = timeToMinutes(endTime);

        return isValidCode(timeSlotId) &&
               isValidDate(examDate) &&
               start >= 0 &&
               end >= 0 &&
               end > start;
    }

    // Lay ma ca thi
    string getTimeSlotId() const {
        return timeSlotId;
    }

    // Lay ngay thi
    string getExamDate() const {
        return examDate;
    }

    // Lay gio bat dau
    string getStartTime() const {
        return startTime;
    }

    // Lay gio ket thuc
    string getEndTime() const {
        return endTime;
    }

    // Kiem tra hai ca thi co trung thoi gian khong
    bool overlaps(const TimeSlot& other) const {
        if (!isValid() || !other.isValid()) {
            return false;
        }

        if (examDate != other.examDate) {
            return false;
        }

        int start1 = timeToMinutes(startTime);
        int end1 = timeToMinutes(endTime);
        int start2 = timeToMinutes(other.startTime);
        int end2 = timeToMinutes(other.endTime);

        return start1 < end2 && start2 < end1;
    }
};

// ==================== CLASS SCHEDULE ====================

class Schedule {
private:
    string scheduleId;
    Course course;
    ExamRoom room;
    TimeSlot timeSlot;
    vector<Student> students;

public:
    Schedule() {
        scheduleId = "";
    }

    Schedule(string id, Course c, ExamRoom r, TimeSlot t) {
        scheduleId = id;
        course = c;
        room = r;
        timeSlot = t;
    }

    bool isValid() const {
        return isValidCode(scheduleId) &&
               course.isValid() &&
               room.isValid() &&
               timeSlot.isValid() &&
               hasEnoughDuration();
    }

    string getScheduleId() const {
        return scheduleId;
    }

    Course getCourse() const {
        return course;
    }

    ExamRoom getRoom() const {
        return room;
    }

    TimeSlot getTimeSlot() const {
        return timeSlot;
    }

    vector<Student> getStudents() const {
        return students;
    }

    // Kiem tra thoi gian thi co du thoi luong mon hoc
    bool hasEnoughDuration() const {
        int start = timeToMinutes(timeSlot.getStartTime());
        int end = timeToMinutes(timeSlot.getEndTime());

        if (start < 0 || end < 0 || end <= start) {
            return false;
        }

        return end - start >= course.getDuration();
    }

    // Them sinh vien neu tat ca dieu kien deu hop le
    bool addStudent(const Student& student) {
        if (!isValid() || !student.isValid()) {
            return false;
        }

        // Sinh vien phai dang ky mon thi
        if (!student.hasRegisteredCourse(course.getCourseId())) {
            return false;
        }

        // Khong cho phep trung sinh vien trong lich thi
        for (int i = 0; i < (int)students.size(); i++) {
            if (students[i].getStudentId() ==
                student.getStudentId()) {
                return false;
            }
        }

        // Khong duoc vuot qua suc chua phong
        if ((int)students.size() >= room.getCapacity()) {
            return false;
        }

        students.push_back(student);
        return true;
    }

    // Kiem tra hai lich co dung cung phong va trung gio
    bool conflictsWithRoom(const Schedule& other) const {
        return room.getRoomId() == other.room.getRoomId() &&
               timeSlot.overlaps(other.timeSlot);
    }

    // Kiem tra hai lich co sinh vien chung khong
    bool hasCommonStudent(const Schedule& other) const {
        for (int i = 0; i < (int)students.size(); i++) {
            for (int j = 0;
                 j < (int)other.students.size(); j++) {
                if (students[i].getStudentId() ==
                    other.students[j].getStudentId()) {
                    return true;
                }
            }
        }

        return false;
    }

    // Kiem tra sinh vien co bi trung lich thi khong
    bool conflictsWithStudent(const Schedule& other) const {
        return timeSlot.overlaps(other.timeSlot) &&
               hasCommonStudent(other);
    }
};
