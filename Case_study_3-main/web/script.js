let dataLoaded = false;
let schedules = [];

const algorithmNames = {
    greedy: "Greedy",
    backtracking: "Backtracking",
    dp: "Dynamic Programming"
};

async function apiRequest(url, options = {}) {
    const response = await fetch(url, options);
    const data = await response.json();

    if (!response.ok || data.ok === false) {
        throw new Error(data.error || "Có lỗi khi kết nối máy chủ.");
    }

    return data;
}

function showPage(page) {
    document.querySelectorAll(".page").forEach(section => {
        section.classList.add("hidden");
    });

    const target = document.getElementById(page);

    if (target) {
        target.classList.remove("hidden");
    }
}

async function loadData() {
    const message = document.getElementById("message");
    message.textContent = "Đang đọc dữ liệu từ C++...";

    try {
        const data = await apiRequest("/api/data");

        document.getElementById("courses").textContent = data.courses;
        document.getElementById("rooms").textContent = data.rooms;
        document.getElementById("slots").textContent = data.slots;
        document.getElementById("total").textContent = "0";

        schedules = [];
        dataLoaded = true;
        renderResults();

        message.textContent =
            `Đọc dữ liệu thật thành công: ${data.courses} môn, ` +
            `${data.rooms} phòng, ${data.slots} ca thi.`;

    } catch (error) {
        dataLoaded = false;
        message.textContent = "Không đọc được dữ liệu: " + error.message;
        alert("Lỗi đọc dữ liệu: " + error.message);
    }
}

async function runAlgorithm(algorithm) {
    if (!dataLoaded) {
        alert("Bà bấm Đọc dữ liệu trước nha!");
        return;
    }

    if (!algorithmNames[algorithm]) {
        alert("Thuật toán không hợp lệ!");
        return;
    }

    const message = document.getElementById("message");
    message.textContent = `Đang chạy thuật toán ${algorithmNames[algorithm]}...`;

    try {
        const data = await apiRequest("/api/schedule", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ algorithm })
        });

        schedules = data.schedules || [];

        document.getElementById("total").textContent = schedules.length;

        renderResults(data.failedCourses || []);

        const failedCount = (data.failedCourses || []).length;

message.textContent =
    `Thuật toán ${data.algorithm} đã chạy xong! ` +
    `Xếp được ${schedules.length} lịch thi. ` +
    `Chưa xếp được: ${failedCount} môn. ` +
    `Thời gian chạy: ${Number(data.runtimeMs).toFixed(3)} ms.`;

        showPage("home");

    } catch (error) {
        message.textContent = "Không chạy được thuật toán: " + error.message;
        alert("Lỗi chạy thuật toán: " + error.message);
    }
}

function renderResults(failedCourses = []) {
    const result = document.getElementById("result");
    const scheduleResult = document.getElementById("schedule-result");

    result.replaceChildren();

    if (scheduleResult) {
        scheduleResult.replaceChildren();
    }

    if (schedules.length === 0) {
        const empty = document.createElement("p");
        empty.textContent = "Chưa có lịch thi được tạo.";
        result.appendChild(empty);

        if (scheduleResult) {
            scheduleResult.appendChild(empty.cloneNode(true));
        }
    }

    schedules.forEach(schedule => {
        const row = document.createElement("div");
        row.className = "result-row";

        const info = document.createElement("div");

        const title = document.createElement("strong");
        title.textContent =
            `${schedule.courseId || ""} - ${schedule.courseName || "Môn thi"}`;

        const details = document.createElement("p");
        details.textContent =
            `Mã lịch: ${schedule.id || ""} | ` +
            `Phòng: ${schedule.roomId || "Chưa rõ"} | ` +
            `Ngày: ${schedule.date || "Chưa rõ"} | ` +
            `Giờ: ${schedule.start || "?"} - ${schedule.end || "?"}`;

        const students = document.createElement("p");
        students.textContent = `Số sinh viên: ${schedule.studentCount ?? 0}`;

        info.append(title, details, students);
        row.appendChild(info);

        result.appendChild(row);

        if (scheduleResult) {
            scheduleResult.appendChild(row.cloneNode(true));
        }
    });

    if (failedCourses.length > 0) {
        const warning = document.createElement("div");
        warning.className = "result-row";

        const text = document.createElement("p");
        text.textContent =
            "Các môn chưa xếp được: " + failedCourses.join(", ");

        warning.appendChild(text);
        result.appendChild(warning);

        if (scheduleResult) {
            scheduleResult.appendChild(warning.cloneNode(true));
        }
    }
}