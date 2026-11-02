#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <winsock2.h>

#include "DataProcessor.h"
#include "GreedyScheduler.h"
#include "BacktrackingScheduler.h"
#include "DPScheduler.h"

#pragma comment(lib, "ws2_32")

using namespace std;

// ======================================================
// CHUYEN CHUOI THANH JSON STRING AN TOAN
// ======================================================

string escapeJSON(const string& s) {
    string out;

    for (unsigned char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;

            case '\\':
                out += "\\\\";
                break;

            case '\b':
                out += "\\b";
                break;

            case '\f':
                out += "\\f";
                break;

            case '\n':
                out += "\\n";
                break;

            case '\r':
                out += "\\r";
                break;

            case '\t':
                out += "\\t";
                break;

            default:
                if (c < 0x20) {
                    const char hex[] = "0123456789abcdef";

                    out += "\\u00";
                    out += hex[(c >> 4) & 0x0F];
                    out += hex[c & 0x0F];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }

    return out;
}

// ======================================================
// TAO HTTP RESPONSE
// ======================================================

string makeResponse(
    const string& body,
    const string& type = "application/json",
    const string& status = "200 OK"
) {
    stringstream ss;

    ss << "HTTP/1.1 " << status << "\r\n"
       << "Content-Type: " << type << "; charset=utf-8\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
       << "Access-Control-Allow-Headers: Content-Type\r\n"
       << "Connection: close\r\n"
       << "Content-Length: " << body.size() << "\r\n\r\n"
       << body;

    return ss.str();
}

void sendText(SOCKET client, const string& text) {
    size_t sent = 0;

    while (sent < text.size()) {
        int result = send(
            client,
            text.c_str() + sent,
            static_cast<int>(text.size() - sent),
            0
        );

        if (result == SOCKET_ERROR || result == 0) {
            break;
        }

        sent += result;
    }
}

// ======================================================
// API DOC DU LIEU VA TRA VE THONG KE CHI TIET
// GET /api/data
// ======================================================

string getJSONData() {
    DataProcessor dp;

    if (!dp.loadFromFile("data/data_Config")) {
        return
            R"({"ok":false,"error":"Khong doc duoc du lieu. Hay chay server tu thu muc goc project."})";
    }

    auto courses = dp.getCourses();
    auto students = dp.getStudents();
    auto rooms = dp.getRooms();
    auto slots = dp.getTimeSlots();

    // Dem so sinh vien dang ky tung mon.
    unordered_map<string, int> courseStudentCount;

    for (const auto& student : students) {
        for (const auto& courseId : student.getRegisteredCourses()) {
            courseStudentCount[courseId]++;
        }
    }

    ostringstream json;

    json << "{\"ok\":true";

    json << ",\"courses\":" << courses.size();
    json << ",\"rooms\":" << rooms.size();
    json << ",\"slots\":" << slots.size();
    json << ",\"students\":" << students.size();

    // --------------------------------------------------
    // DANH SACH MON HOC
    // --------------------------------------------------

    json << ",\"courseList\":[";

    for (size_t i = 0; i < courses.size(); ++i) {
        if (i > 0) {
            json << ",";
        }

        const auto& course = courses[i];

        string courseId = course.getCourseId();

        json << "{"
             << "\"id\":\"" << escapeJSON(courseId) << "\","
             << "\"courseId\":\"" << escapeJSON(courseId) << "\","
             << "\"name\":\""
             << escapeJSON(course.getCourseName()) << "\","
             << "\"courseName\":\""
             << escapeJSON(course.getCourseName()) << "\","
             << "\"duration\":" << course.getDuration() << ","
             << "\"studentCount\":"
             << courseStudentCount[courseId]
             << "}";
    }

    json << "]";

    // --------------------------------------------------
    // DANH SACH PHONG THI
    // --------------------------------------------------

    json << ",\"roomList\":[";

    for (size_t i = 0; i < rooms.size(); ++i) {
        if (i > 0) {
            json << ",";
        }

        const auto& room = rooms[i];

        json << "{"
             << "\"id\":\"" << escapeJSON(room.getRoomId()) << "\","
             << "\"roomId\":\"" << escapeJSON(room.getRoomId()) << "\","
             << "\"capacity\":" << room.getCapacity()
             << "}";
    }

    json << "]";

    // --------------------------------------------------
    // DANH SACH CA THI
    // --------------------------------------------------

    json << ",\"slotList\":[";

    for (size_t i = 0; i < slots.size(); ++i) {
        if (i > 0) {
            json << ",";
        }

        const auto& slot = slots[i];

        json << "{"
             << "\"id\":\""
             << escapeJSON(slot.getTimeSlotId()) << "\","
             << "\"timeSlotId\":\""
             << escapeJSON(slot.getTimeSlotId()) << "\","
             << "\"date\":\""
             << escapeJSON(slot.getExamDate()) << "\","
             << "\"examDate\":\""
             << escapeJSON(slot.getExamDate()) << "\","
             << "\"start\":\""
             << escapeJSON(slot.getStartTime()) << "\","
             << "\"startTime\":\""
             << escapeJSON(slot.getStartTime()) << "\","
             << "\"end\":\""
             << escapeJSON(slot.getEndTime()) << "\","
             << "\"endTime\":\""
             << escapeJSON(slot.getEndTime()) << "\""
             << "}";
    }

    json << "]}";

    return json.str();
}

// ======================================================
// CHAY THUAT TOAN VA TRA VE LICH THI
// POST /api/schedule
// ======================================================

string runSchedule(const string& body) {
    string algorithm;

    // Doc ten thuat toan tu JSON gui len.
    if (body.find("\"backtracking\"") != string::npos) {
        algorithm = "backtracking";
    } else if (body.find("\"greedy\"") != string::npos) {
        algorithm = "greedy";
    } else if (body.find("\"dp\"") != string::npos) {
        algorithm = "dp";
    } else {
        return
            R"({"ok":false,"error":"Thuat toan khong hop le"})";
    }

    DataProcessor dp;

    if (!dp.loadFromFile("data/data_Config")) {
        return
            R"({"ok":false,"error":"Khong doc duoc du lieu"})";
    }

    auto courses = dp.getCourses();
    auto rooms = dp.getRooms();
    auto slots = dp.getTimeSlots();
    auto students = dp.getStudents();

    // Gom sinh vien theo mon hoc.
    unordered_map<string, vector<Student>> courseStudentsMap;

    for (const auto& student : students) {
        for (const auto& courseId : student.getRegisteredCourses()) {
            courseStudentsMap[courseId].push_back(student);
        }
    }

    vector<string> failedCourses;
    vector<Schedule> schedules;

    auto startTime = chrono::steady_clock::now();

    if (algorithm == "greedy") {
        schedules = GreedyScheduler::schedule(
            courses,
            rooms,
            slots,
            courseStudentsMap,
            failedCourses
        );
    } else if (algorithm == "backtracking") {
        schedules = BacktrackingScheduler::schedule(
            courses,
            rooms,
            slots,
            courseStudentsMap,
            failedCourses
        );
    } else {
        schedules = DPScheduler::runDPScheduler(
            courses,
            rooms,
            slots,
            courseStudentsMap,
            failedCourses
        );
    }

    auto endTime = chrono::steady_clock::now();

    double elapsedMs =
        chrono::duration<double, milli>(
            endTime - startTime
        ).count();

    string algorithmName =
        algorithm == "greedy" ? "Greedy" :
        algorithm == "backtracking" ? "Backtracking" :
        "Dynamic Programming";

    // --------------------------------------------------
    // THONG KE THEO PHONG VA CA THI
    // --------------------------------------------------

    struct Usage {
        int scheduleCount = 0;
        int studentCount = 0;
    };

    map<string, Usage> roomUsage;
    map<string, Usage> slotUsage;

    for (const auto& schedule : schedules) {
        string roomId = schedule.getRoom().getRoomId();
        string slotId = schedule.getTimeSlot().getTimeSlotId();

        int studentCount =
            static_cast<int>(schedule.getStudents().size());

        roomUsage[roomId].scheduleCount++;
        roomUsage[roomId].studentCount += studentCount;

        slotUsage[slotId].scheduleCount++;
        slotUsage[slotId].studentCount += studentCount;
    }

    // --------------------------------------------------
    // DONG GOI JSON
    // --------------------------------------------------

    ostringstream json;

    json << "{\"ok\":true";
    json << ",\"algorithm\":\"" << algorithmName << "\"";
    json << ",\"runtimeMs\":" << elapsedMs;
    json << ",\"total\":" << schedules.size();
    json << ",\"failedCount\":" << failedCourses.size();

    // --------------------------------------------------
    // DANH SACH LICH THI
    // --------------------------------------------------

    json << ",\"schedules\":[";

    for (size_t i = 0; i < schedules.size(); ++i) {
        if (i > 0) {
            json << ",";
        }

        const auto& schedule = schedules[i];

        json << "{"
             << "\"id\":\""
             << escapeJSON(schedule.getScheduleId()) << "\","

             << "\"courseId\":\""
             << escapeJSON(
                    schedule.getCourse().getCourseId()
                ) << "\","

             << "\"courseName\":\""
             << escapeJSON(
                    schedule.getCourse().getCourseName()
                ) << "\","

             << "\"roomId\":\""
             << escapeJSON(
                    schedule.getRoom().getRoomId()
                ) << "\","

             << "\"date\":\""
             << escapeJSON(
                    schedule.getTimeSlot().getExamDate()
                ) << "\","

             << "\"start\":\""
             << escapeJSON(
                    schedule.getTimeSlot().getStartTime()
                ) << "\","

             << "\"end\":\""
             << escapeJSON(
                    schedule.getTimeSlot().getEndTime()
                ) << "\","

             << "\"studentCount\":"
             << schedule.getStudents().size()

             << "}";
    }

    json << "]";

    // --------------------------------------------------
    // CAC MON CHUA XEP DUOC
    // --------------------------------------------------

    json << ",\"failedCourses\":[";

    for (size_t i = 0; i < failedCourses.size(); ++i) {
        if (i > 0) {
            json << ",";
        }

        json << "\""
             << escapeJSON(failedCourses[i])
             << "\"";
    }

    json << "]";

    // --------------------------------------------------
    // THONG KE THEO PHONG
    // --------------------------------------------------

    json << ",\"roomStatistics\":[";

    bool first = true;

    for (const auto& entry : roomUsage) {
        if (!first) {
            json << ",";
        }

        first = false;

        json << "{"
             << "\"id\":\"" << escapeJSON(entry.first) << "\","
             << "\"scheduleCount\":"
             << entry.second.scheduleCount << ","
             << "\"studentCount\":"
             << entry.second.studentCount
             << "}";
    }

    json << "]";

    // --------------------------------------------------
    // THONG KE THEO CA THI
    // --------------------------------------------------

    json << ",\"slotStatistics\":[";

    first = true;

    for (const auto& entry : slotUsage) {
        if (!first) {
            json << ",";
        }

        first = false;

        json << "{"
             << "\"id\":\"" << escapeJSON(entry.first) << "\","
             << "\"scheduleCount\":"
             << entry.second.scheduleCount << ","
             << "\"studentCount\":"
             << entry.second.studentCount
             << "}";
    }

    json << "]}";

    return json.str();
}

// ======================================================
// DOC NOI DUNG HTTP REQUEST
// ======================================================

string getRequestBody(const string& request) {
    size_t bodyPos = request.find("\r\n\r\n");

    if (bodyPos == string::npos) {
        return "";
    }

    string body = request.substr(bodyPos + 4);

    // Neu request duoc nhan thanh nhieu lan thi phan xu ly
    // Content-Length day du can duoc bo sung rieng.
    return body;
}

// ======================================================
// MAIN SERVER
// ======================================================

int main() {
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "Khong khoi dong duoc Winsock.\n";
        return 1;
    }

    SOCKET serverSocket =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        cerr << "Khong tao duoc socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) == SOCKET_ERROR) {

        cerr << "Khong bind duoc cong 8080. "
             << "Co the cong dang duoc su dung.\n";

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Khong the lang nghe ket noi.\n";

        closesocket(serverSocket);
        WSACleanup();

        return 1;
    }

    cout << "Server dang chay tai http://127.0.0.1:8080\n";
    cout << "Nhan Ctrl+C de dung.\n";

    while (true) {
        SOCKET client =
            accept(serverSocket, nullptr, nullptr);

        if (client == INVALID_SOCKET) {
            continue;
        }

        // Doc request. Buffer lon hon ban cu.
        char buffer[65536]{};

        int received = recv(
            client,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (received <= 0) {
            closesocket(client);
            continue;
        }

        string request(buffer, received);

        size_t lineEnd = request.find("\r\n");

        string firstLine =
            request.substr(0, lineEnd);

        string path = "/";

        size_t firstSpace = firstLine.find(' ');
        size_t secondSpace =
            firstLine.find(' ', firstSpace + 1);

        if (firstSpace != string::npos &&
            secondSpace != string::npos) {

            path = firstLine.substr(
                firstSpace + 1,
                secondSpace - firstSpace - 1
            );
        }

        string response;

        // Xu ly OPTIONS cho CORS.
        if (firstLine.find("OPTIONS ") == 0) {
            response = makeResponse("", "text/plain");
        }

        // API thong ke.
        else if (path.find("/api/data") == 0) {
            response = makeResponse(getJSONData());
        }

        // API chay thuat toan.
        else if (
            path == "/api/schedule" &&
            firstLine.find("POST ") == 0
        ) {
            string body = getRequestBody(request);

            response = makeResponse(runSchedule(body));
        }

        // Tai giao dien.
        else {
            string filePath;

            if (path == "/" || path == "/index.html") {
                filePath = "web/index.html";
            } else if (path == "/script.js") {
                filePath = "web/script.js";
            } else if (path == "/style.css") {
                filePath = "web/style.css";
            }

            if (filePath.empty()) {
                response =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
            } else {
                ifstream file(filePath, ios::binary);

                if (!file) {
                    response =
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n\r\n";
                } else {
                    stringstream content;
                    content << file.rdbuf();

                    string type =
                        filePath.find(".css") != string::npos
                        ? "text/css" :
                        filePath.find(".js") != string::npos
                        ? "application/javascript" :
                        "text/html";

                    response = makeResponse(
                        content.str(),
                        type
                    );
                }
            }
        }

        sendText(client, response);

        shutdown(client, SD_SEND);
        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}