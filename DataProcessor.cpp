#include "DataProcessor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

// ==================== CAC HAM HO TRO ====================

// Tach mot dong thanh cac phan dua vao ky tu phan cach.
// Vi du: "C01|Cau truc du lieu|90"
// se tach thanh 3 phan.
vector<string> splitString(const string& s, char kyTu) {
    vector<string> ketQua;
    string phan;
    stringstream ss(s);

    while (getline(ss, phan, kyTu)) {
        ketQua.push_back(phan);
    }

    // Xu ly truong hop ky tu phan cach nam o cuoi dong
    if (!s.empty() && s[s.length() - 1] == kyTu) {
        ketQua.push_back("");
    }

    return ketQua;
}

// Kiem tra mot dong co rong hay chi chua khoang trang khong
bool isBlankLine(const string& s) {
    return normalizeName(s).empty();
}

// Chuyen chuoi thanh so nguyen.
// Tra ve false neu chuoi khong phai so nguyen hop le.
bool parseInteger(const string& s, int& giaTri) {
    stringstream ss(s);
    char kyTuThua;

    if (!(ss >> giaTri)) {
        return false;
    }

    if (ss >> kyTuThua) {
        return false;
    }

    return true;
}

// ==================== HAM KHOI TAO ====================

DataProcessor::DataProcessor() {
    courseCount = 0;
    studentCount = 0;
    roomCount = 0;
    slotCount = 0;
}

// ==================== DOC DU LIEU ====================

// filename la duong dan den file cau hinh.
// Vi du: data/data_Config
bool DataProcessor::loadFromFile(const string& filename) {
    // Xoa du lieu cu neu ham duoc goi lai
    courses.clear();
    students.clear();
    rooms.clear();
    timeSlots.clear();
    conflictGraph.clear();

    courseCount = 0;
    studentCount = 0;
    roomCount = 0;
    slotCount = 0;

    // Tim thu muc chua file cau hinh
    string thuMuc = "";
    size_t viTri = filename.find_last_of("/\\");

    if (viTri != string::npos) {
        thuMuc = filename.substr(0, viTri + 1);
    }

    // Tao duong dan den cac file du lieu
    string fileConfig = filename;
    string fileCourses = thuMuc + "data_Courses.txt";
    string fileStudents = thuMuc + "data_Students.txt";
    string fileRooms = thuMuc + "data_Rooms.txt";
    string fileSlots = thuMuc + "data_Time_Slots.txt";

    // ---------- 1. DOC FILE CONFIG ----------

    ifstream file(fileConfig.c_str());

    if (!file.is_open()) {
        cerr << "Loi: Khong mo duoc file cau hinh: "
             << fileConfig << endl;
        return false;
    }

    string dong;
    int soDong = 0;

    while (getline(file, dong)) {
        soDong++;

        if (isBlankLine(dong)) {
            continue;
        }

        vector<string> phan = splitString(dong, '=');

        if (phan.size() != 2) {
            cerr << "Loi file CONFIG, dong "
                 << soDong << ": Sai dinh dang." << endl;
            file.close();
            return false;
        }

        string ten = normalizeName(phan[0]);
        string giaTriChuoi = normalizeName(phan[1]);
        int giaTri;

        if (!parseInteger(giaTriChuoi, giaTri) ||
            giaTri <= 0) {
            cerr << "Loi file CONFIG, dong "
                 << soDong
                 << ": So luong phai la so nguyen duong."
                 << endl;
            file.close();
            return false;
        }

        if (ten == "COURSE_COUNT") {
            if (courseCount != 0) {
                cerr << "Loi: COURSE_COUNT bi khai bao trung."
                     << endl;
                file.close();
                return false;
            }
            courseCount = giaTri;
        }
        else if (ten == "STUDENT_COUNT") {
            if (studentCount != 0) {
                cerr << "Loi: STUDENT_COUNT bi khai bao trung."
                     << endl;
                file.close();
                return false;
            }
            studentCount = giaTri;
        }
        else if (ten == "ROOM_COUNT") {
            if (roomCount != 0) {
                cerr << "Loi: ROOM_COUNT bi khai bao trung."
                     << endl;
                file.close();
                return false;
            }
            roomCount = giaTri;
        }
        else if (ten == "SLOT_COUNT") {
            if (slotCount != 0) {
                cerr << "Loi: SLOT_COUNT bi khai bao trung."
                     << endl;
                file.close();
                return false;
            }
            slotCount = giaTri;
        }
        else {
            cerr << "Loi file CONFIG, dong "
                 << soDong << ": Ten cau hinh khong hop le."
                 << endl;
            file.close();
            return false;
        }
    }

    file.close();

    // Dam bao CONFIG co du 4 thong so
    if (courseCount == 0 || studentCount == 0 ||
        roomCount == 0 || slotCount == 0) {
        cerr << "Loi: File CONFIG bi thieu thong tin."
             << endl;
        return false;
    }

    // ---------- 2. DOC DANH SACH MON HOC ----------

    file.open(fileCourses.c_str());

    if (!file.is_open()) {
        cerr << "Loi: Khong mo duoc file "
             << fileCourses << endl;
        return false;
    }

    soDong = 0;

    while (getline(file, dong)) {
        soDong++;

        if (isBlankLine(dong)) {
            continue;
        }

        vector<string> phan = splitString(dong, '|');

        if (phan.size() != 3) {
            cerr << "Loi file COURSES, dong "
                 << soDong << ": Can 3 truong du lieu."
                 << endl;
            file.close();
            return false;
        }

        string maMon = normalizeName(phan[0]);
        string tenMon = normalizeName(phan[1]);
        string thoiLuongChuoi = normalizeName(phan[2]);
        int thoiLuong;

        if (maMon.empty() || tenMon.empty() ||
            thoiLuongChuoi.empty()) {
            cerr << "Loi file COURSES, dong "
                 << soDong << ": Du lieu bi thieu."
                 << endl;
            file.close();
            return false;
        }

        if (!parseInteger(thoiLuongChuoi, thoiLuong)) {
            cerr << "Loi file COURSES, dong "
                 << soDong << ": Thoi luong khong hop le."
                 << endl;
            file.close();
            return false;
        }

        // Kiem tra ma mon co bi trung khong
        for (int i = 0; i < (int)courses.size(); i++) {
            if (courses[i].getCourseId() ==
                normalizeCode(maMon)) {
                cerr << "Loi file COURSES, dong "
                     << soDong << ": Ma mon bi trung."
                     << endl;
                file.close();
                return false;
            }
        }

        Course mon(maMon, tenMon, thoiLuong);

        if (!mon.isValid()) {
            cerr << "Loi file COURSES, dong "
                 << soDong << ": Thong tin mon hoc khong hop le."
                 << endl;
            file.close();
            return false;
        }

        courses.push_back(mon);
    }

    file.close();

    // ---------- 3. DOC DANH SACH SINH VIEN ----------

    file.open(fileStudents.c_str());

    if (!file.is_open()) {
        cerr << "Loi: Khong mo duoc file "
             << fileStudents << endl;
        return false;
    }

    soDong = 0;

    while (getline(file, dong)) {
        soDong++;

        if (isBlankLine(dong)) {
            continue;
        }

        vector<string> phan = splitString(dong, '|');

        if (phan.size() != 5) {
            cerr << "Loi file STUDENTS, dong "
                 << soDong << ": Can 5 truong du lieu."
                 << endl;
            file.close();
            return false;
        }

        string maSV = normalizeName(phan[0]);
        string hoTen = normalizeName(phan[1]);
        string ngaySinh = normalizeName(phan[2]);
        string gioiTinh = normalizeName(phan[3]);
        string danhSachMon = normalizeName(phan[4]);

        if (maSV.empty() || hoTen.empty() ||
            ngaySinh.empty() || gioiTinh.empty() ||
            danhSachMon.empty()) {
            cerr << "Loi file STUDENTS, dong "
                 << soDong << ": Du lieu bi thieu."
                 << endl;
            file.close();
            return false;
        }

        // Tach danh sach mon hoc theo dau phay
        vector<string> cacMon =
            splitString(danhSachMon, ',');

        vector<string> monDangKy;

        for (int i = 0; i < (int)cacMon.size(); i++) {
            string maMon = normalizeName(cacMon[i]);

            if (maMon.empty()) {
                cerr << "Loi file STUDENTS, dong "
                     << soDong
                     << ": Ma mon dang ky bi thieu."
                     << endl;
                file.close();
                return false;
            }

            monDangKy.push_back(maMon);
        }

        // Kiem tra ma sinh vien bi trung
        for (int i = 0; i < (int)students.size(); i++) {
            if (students[i].getStudentId() ==
                normalizeCode(maSV)) {
                cerr << "Loi file STUDENTS, dong "
                     << soDong << ": Ma sinh vien bi trung."
                     << endl;
                file.close();
                return false;
            }
        }

        Student sinhVien(
            maSV, hoTen, ngaySinh, gioiTinh, monDangKy
        );

        if (!sinhVien.isValid()) {
            cerr << "Loi file STUDENTS, dong "
                 << soDong
                 << ": Thong tin sinh vien khong hop le."
                 << endl;
            file.close();
            return false;
        }

        students.push_back(sinhVien);
    }

    file.close();

    // ---------- 4. DOC DANH SACH PHONG THI ----------

    file.open(fileRooms.c_str());

    if (!file.is_open()) {
        cerr << "Loi: Khong mo duoc file "
             << fileRooms << endl;
        return false;
    }

    soDong = 0;

    while (getline(file, dong)) {
        soDong++;

        if (isBlankLine(dong)) {
            continue;
        }

        vector<string> phan = splitString(dong, '|');

        if (phan.size() != 2) {
            cerr << "Loi file ROOMS, dong "
                 << soDong << ": Can 2 truong du lieu."
                 << endl;
            file.close();
            return false;
        }

        string maPhong = normalizeName(phan[0]);
        string sucChuaChuoi = normalizeName(phan[1]);
        int sucChua;

        if (maPhong.empty() || sucChuaChuoi.empty()) {
            cerr << "Loi file ROOMS, dong "
                 << soDong << ": Du lieu bi thieu."
                 << endl;
            file.close();
            return false;
        }

        if (!parseInteger(sucChuaChuoi, sucChua)) {
            cerr << "Loi file ROOMS, dong "
                 << soDong << ": Suc chua khong hop le."
                 << endl;
            file.close();
            return false;
        }

        // Kiem tra ma phong bi trung
        for (int i = 0; i < (int)rooms.size(); i++) {
            if (rooms[i].getRoomId() ==
                normalizeCode(maPhong)) {
                cerr << "Loi file ROOMS, dong "
                     << soDong << ": Ma phong bi trung."
                     << endl;
                file.close();
                return false;
            }
        }

        ExamRoom phong(maPhong, sucChua);

        if (!phong.isValid()) {
            cerr << "Loi file ROOMS, dong "
                 << soDong
                 << ": Ma phong hoac suc chua khong hop le."
                 << endl;
            file.close();
            return false;
        }

        rooms.push_back(phong);
    }

    file.close();

    // ---------- 5. DOC DANH SACH KHUNG GIO ----------

    file.open(fileSlots.c_str());

    if (!file.is_open()) {
        cerr << "Loi: Khong mo duoc file "
             << fileSlots << endl;
        return false;
    }

    soDong = 0;

    while (getline(file, dong)) {
        soDong++;

        if (isBlankLine(dong)) {
            continue;
        }

        vector<string> phan = splitString(dong, '|');

        if (phan.size() != 4) {
            cerr << "Loi file TIME_SLOTS, dong "
                 << soDong << ": Can 4 truong du lieu."
                 << endl;
            file.close();
            return false;
        }

        string maKhungGio = normalizeName(phan[0]);
        string ngayThi = normalizeName(phan[1]);
        string gioBatDau = normalizeName(phan[2]);
        string gioKetThuc = normalizeName(phan[3]);

        if (maKhungGio.empty() || ngayThi.empty() ||
            gioBatDau.empty() || gioKetThuc.empty()) {
            cerr << "Loi file TIME_SLOTS, dong "
                 << soDong << ": Du lieu bi thieu."
                 << endl;
            file.close();
            return false;
        }

        // Kiem tra ma khung gio bi trung
        for (int i = 0; i < (int)timeSlots.size(); i++) {
            if (timeSlots[i].getTimeSlotId() ==
                normalizeCode(maKhungGio)) {
                cerr << "Loi file TIME_SLOTS, dong "
                     << soDong << ": Ma khung gio bi trung."
                     << endl;
                file.close();
                return false;
            }
        }

        TimeSlot khungGio(
            maKhungGio, ngayThi, gioBatDau, gioKetThuc
        );

        if (!khungGio.isValid()) {
            cerr << "Loi file TIME_SLOTS, dong "
                 << soDong
                 << ": Khung gio khong hop le."
                 << endl;
            file.close();
            return false;
        }

        timeSlots.push_back(khungGio);
    }

    file.close();

    cout << "Da doc xong cac file du lieu." << endl;

    return true;
}

// ==================== KIEM TRA DU LIEU ====================

bool DataProcessor::validateData() {
    bool hopLe = true;

    // Kiem tra so luong du lieu voi CONFIG
    if ((int)courses.size() != courseCount) {
        cerr << "Loi: So mon hoc thuc te la "
             << courses.size()
             << ", CONFIG khai bao "
             << courseCount << "." << endl;
        hopLe = false;
    }

    if ((int)students.size() != studentCount) {
        cerr << "Loi: So sinh vien thuc te la "
             << students.size()
             << ", CONFIG khai bao "
             << studentCount << "." << endl;
        hopLe = false;
    }

    if ((int)rooms.size() != roomCount) {
        cerr << "Loi: So phong thi thuc te la "
             << rooms.size()
             << ", CONFIG khai bao "
             << roomCount << "." << endl;
        hopLe = false;
    }

    if ((int)timeSlots.size() != slotCount) {
        cerr << "Loi: So khung gio thuc te la "
             << timeSlots.size()
             << ", CONFIG khai bao "
             << slotCount << "." << endl;
        hopLe = false;
    }

    // Kiem tra co du lieu can thiet hay khong
    if (courses.empty()) {
        cerr << "Loi: Khong co mon hoc nao." << endl;
        hopLe = false;
    }

    if (students.empty()) {
        cerr << "Loi: Khong co sinh vien nao." << endl;
        hopLe = false;
    }

    if (rooms.empty()) {
        cerr << "Loi: Khong co phong thi nao." << endl;
        hopLe = false;
    }

    if (timeSlots.empty()) {
        cerr << "Loi: Khong co khung gio thi nao." << endl;
        hopLe = false;
    }

    // Kiem tra cac mon sinh vien dang ky co ton tai khong
    for (int i = 0; i < (int)students.size(); i++) {
        vector<string> monDangKy =
            students[i].getRegisteredCourses();

        for (int j = 0; j < (int)monDangKy.size(); j++) {
            bool timThay = false;

            for (int k = 0; k < (int)courses.size(); k++) {
                if (monDangKy[j] ==
                    courses[k].getCourseId()) {
                    timThay = true;
                    break;
                }
            }

            if (!timThay) {
                cerr << "Loi: Sinh vien "
                     << students[i].getStudentId()
                     << " dang ky mon "
                     << monDangKy[j]
                     << " khong ton tai." << endl;
                hopLe = false;
            }
        }
    }

    return hopLe;
}

// ==================== XAY DUNG DO THI XUNG DOT ====================

void DataProcessor::buildConflictGraph() {
    conflictGraph.clear();

    // Tao dinh cho tat ca cac mon hoc
    for (int i = 0; i < (int)courses.size(); i++) {
        string maMon = courses[i].getCourseId();
        conflictGraph[maMon] = vector<string>();
    }

    // Xet tung sinh vien
    // Cac mon sinh vien dang ky se xung dot voi nhau
    for (int i = 0; i < (int)students.size(); i++) {
        vector<string> monDangKy =
            students[i].getRegisteredCourses();

        for (int j = 0; j < (int)monDangKy.size(); j++) {
            for (int k = j + 1;
                 k < (int)monDangKy.size(); k++) {

                string mon1 = monDangKy[j];
                string mon2 = monDangKy[k];

                // Chi them canh neu ca hai mon ton tai
                if (conflictGraph.find(mon1) !=
                        conflictGraph.end() &&
                    conflictGraph.find(mon2) !=
                        conflictGraph.end()) {

                    // Them mon2 vao danh sach xung dot cua mon1
                    vector<string>& ds1 = conflictGraph[mon1];
                    bool daCo = false;

                    for (int a = 0; a < (int)ds1.size(); a++) {
                        if (ds1[a] == mon2) {
                            daCo = true;
                            break;
                        }
                    }

                    if (!daCo) {
                        ds1.push_back(mon2);
                    }

                    // Them mon1 vao danh sach xung dot cua mon2
                    vector<string>& ds2 = conflictGraph[mon2];
                    daCo = false;

                    for (int a = 0; a < (int)ds2.size(); a++) {
                        if (ds2[a] == mon1) {
                            daCo = true;
                            break;
                        }
                    }

                    if (!daCo) {
                        ds2.push_back(mon1);
                    }
                }
            }
        }
    }
}

// ==================== CAC HAM LAY DU LIEU ====================

vector<Course> DataProcessor::getCourses() const {
    return courses;
}

vector<Student> DataProcessor::getStudents() const {
    return students;
}

vector<ExamRoom> DataProcessor::getRooms() const {
    return rooms;
}

vector<TimeSlot> DataProcessor::getTimeSlots() const {
    return timeSlots;
}

map<string, vector<string> >
DataProcessor::getConflictGraph() const {
    return conflictGraph;
}
