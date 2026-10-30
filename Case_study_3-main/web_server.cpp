#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <winsock2.h>

#include "DataProcessor.h"
#include "GreedyScheduler.h"
#include "BacktrackingScheduler.h"
#include "DPScheduler.h"

#pragma comment(lib, "ws2_32")

using namespace std;

string escapeJSON(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') {
            out += "\\n";
            continue;
        }
        if (c == '\r') continue;
        out += c;
    }
    return out;
}

string makeResponse(const string& body, const string& type = "application/json") {
    stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: " << type << "; charset=utf-8\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "Connection: close\r\n"
       << "Content-Length: " << body.size() << "\r\n\r\n"
       << body;
    return ss.str();
}

void sendText(SOCKET client, const string& text) {
    send(client, text.c_str(), (int)text.size(), 0);
}

string getJSONData() {
    DataProcessor dp;

    if (!dp.loadFromFile("data/data_Config")) {
        return R"({"ok":false,"error":"Khong doc duoc du lieu. Hay chay server tu thu muc goc project."})";
    }

    return string("{\"ok\":true,\"courses\":") +
        to_string(dp.getCourses().size()) +
        ",\"rooms\":" + to_string(dp.getRooms().size()) +
        ",\"slots\":" + to_string(dp.getTimeSlots().size()) + "}";
}

string runSchedule(const string& body) {
    string algorithm = "greedy";

    if (body.find("backtracking") != string::npos)
        algorithm = "backtracking";
    else if (body.find("\"dp\"") != string::npos)
        algorithm = "dp";
    else if (body.find("greedy") == string::npos)
        return R"({"ok":false,"error":"Thuat toan khong hop le"})";

    DataProcessor dp;
    if (!dp.loadFromFile("data/data_Config")) {
        return R"({"ok":false,"error":"Khong doc duoc du lieu"})";
    }

    auto courses = dp.getCourses();
    auto rooms = dp.getRooms();
    auto slots = dp.getTimeSlots();
    auto students = dp.getStudents();

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
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
} else if (algorithm == "backtracking") {
    schedules = BacktrackingScheduler::schedule(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
} else {
    schedules = DPScheduler::runDPScheduler(
        courses, rooms, slots, courseStudentsMap, failedCourses
    );
}

auto endTime = chrono::steady_clock::now();

double elapsedMs = chrono::duration<double, milli>(
    endTime - startTime
).count();
    string name = algorithm == "greedy" ? "Greedy" :
        algorithm == "backtracking" ? "Backtracking" : "Dynamic Programming";

    ostringstream timeStream;
timeStream << elapsedMs;

string json = "{\"ok\":true,\"algorithm\":\"" + name +
    "\",\"runtimeMs\":" + timeStream.str() +
    ",\"total\":" + to_string(schedules.size()) +
    ",\"failedCount\":" + to_string(failedCourses.size()) +
    ",\"schedules\":[";

    for (size_t i = 0; i < schedules.size(); ++i) {
        const auto& s = schedules[i];

        if (i > 0) json += ",";

        json += "{\"id\":\"" + escapeJSON(s.getScheduleId()) + "\"";
        json += ",\"courseId\":\"" + escapeJSON(s.getCourse().getCourseId()) + "\"";
        json += ",\"courseName\":\"" + escapeJSON(s.getCourse().getCourseName()) + "\"";
        json += ",\"roomId\":\"" + escapeJSON(s.getRoom().getRoomId()) + "\"";
        json += ",\"date\":\"" + escapeJSON(s.getTimeSlot().getExamDate()) + "\"";
        json += ",\"start\":\"" + escapeJSON(s.getTimeSlot().getStartTime()) + "\"";
        json += ",\"end\":\"" + escapeJSON(s.getTimeSlot().getEndTime()) + "\"";
        json += ",\"studentCount\":" + to_string(s.getStudents().size()) + "}";
    }

    json += "],\"failedCourses\":[";

    for (size_t i = 0; i < failedCourses.size(); ++i) {
        if (i > 0) json += ",";
        json += "\"" + escapeJSON(failedCourses[i]) + "\"";
    }

    json += "]}";
    return json;
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "Khong khoi dong duoc Winsock.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Khong tao duoc socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Khong bind duoc cong 8080. Co the cong dang duoc su dung.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, SOMAXCONN);

    cout << "Server dang chay tai http://127.0.0.1:8080\n";
    cout << "Nhan Ctrl+C de dung.\n";

    while (true) {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        char buffer[8192]{};
        int received = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (received <= 0) {
            closesocket(client);
            continue;
        }

        string request(buffer, received);
        string response;

        size_t lineEnd = request.find("\r\n");
        string firstLine = request.substr(0, lineEnd);
        string path = "/";

        size_t firstSpace = firstLine.find(' ');
        size_t secondSpace = firstLine.find(' ', firstSpace + 1);

        if (firstSpace != string::npos && secondSpace != string::npos)
            path = firstLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

        if (path.find("/api/data") == 0) {
            response = makeResponse(getJSONData());
        } else if (path == "/api/schedule" && firstLine.find("POST") == 0) {
            size_t bodyPos = request.find("\r\n\r\n");
            string body = bodyPos == string::npos ? "" : request.substr(bodyPos + 4);
            response = makeResponse(runSchedule(body));
        } else {
            string filePath;

            if (path == "/" || path == "/index.html")
                filePath = "web/index.html";
            else if (path == "/script.js")
                filePath = "web/script.js";
            else if (path == "/style.css")
                filePath = "web/style.css";

            if (filePath.empty()) {
                response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            } else {
                ifstream file(filePath, ios::binary);
                if (!file) {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                } else {
                    stringstream content;
                    content << file.rdbuf();

                    string type = filePath.find(".css") != string::npos ? "text/css" :
                        filePath.find(".js") != string::npos ? "application/javascript" : "text/html";

                    response = makeResponse(content.str(), type);
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