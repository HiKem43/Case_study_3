let dataLoaded = false;
let schedules = [];
let examData = null;
let lastScheduleData = null;

const algorithmNames = {
    greedy: "Greedy",
    backtracking: "Backtracking",
    dp: "Dynamic Programming"
};

// ======================================================
// GOI API C++
// ======================================================

async function apiRequest(url, options = {}) {
    const response = await fetch(url, options);
    const data = await response.json();

    if (!response.ok || data.ok === false) {
        throw new Error(data.error || "Có lỗi khi kết nối máy chủ.");
    }

    return data;
}

// ======================================================
// CHUYEN TRANG
// ======================================================

function showPage(page) {
    document.querySelectorAll(".page").forEach(section => {
        section.classList.add("hidden");
    });

    const target = document.getElementById(page);

    if (target) {
        target.classList.remove("hidden");
    }
}

// ======================================================
// DOC DU LIEU
// ======================================================

async function loadData() {
    const message = document.getElementById("message");

    if (message) {
        message.textContent = "Đang đọc dữ liệu từ C++...";
    }

    try {
        const data = await apiRequest("/api/data");

        examData = data;
        dataLoaded = true;

        document.getElementById("courses").textContent =
            data.courses ?? (data.courseList || []).length;

        document.getElementById("rooms").textContent =
            data.rooms ?? (data.roomList || []).length;

        document.getElementById("slots").textContent =
            data.slots ?? (data.slotList || []).length;

        document.getElementById("total").textContent =
            schedules.length;

        if (message) {
            message.textContent =
                `Đọc dữ liệu thành công: ${data.courses} môn, ` +
                `${data.rooms} phòng, ${data.slots} ca thi.`;
        }

        renderResults();

        // Neu bang thong ke dang mo thi cap nhat lai.
        const detail = document.getElementById("statistics-detail");

        if (detail && !detail.classList.contains("hidden")) {
            const currentType = detail.dataset.type;

            if (currentType) {
                showStatistics(currentType);
            }
        }
    } catch (error) {
        dataLoaded = false;

        if (message) {
            message.textContent =
                "Không đọc được dữ liệu: " + error.message;
        }

        alert("Lỗi đọc dữ liệu: " + error.message);
    }
}

// ======================================================
// CHAY THUAT TOAN
// ======================================================

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

    if (message) {
        message.textContent =
            `Đang chạy thuật toán ${algorithmNames[algorithm]}...`;
    }

    try {
        const data = await apiRequest("/api/schedule", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ algorithm })
        });

        schedules = data.schedules || [];
        lastScheduleData = data;

        document.getElementById("total").textContent =
            schedules.length;

        renderResults(data.failedCourses || []);

        if (message) {
            const failedCount = (data.failedCourses || []).length;

            message.textContent =
                `Thuật toán ${data.algorithm} đã chạy xong! ` +
                `Xếp được ${schedules.length} lịch thi. ` +
                `Chưa xếp được: ${failedCount} môn. ` +
                `Thời gian chạy: ${Number(data.runtimeMs || 0).toFixed(3)} ms.`;
        }

        // Neu dang mo bang thong ke lich thi hoac phong/ca thi,
        // cap nhat bang theo ket qua moi nhat.
        const detail = document.getElementById("statistics-detail");

        if (detail && !detail.classList.contains("hidden")) {
            const currentType = detail.dataset.type;

            if (currentType) {
                showStatistics(currentType);
            }
        }

        showPage("home");
    } catch (error) {
        if (message) {
            message.textContent =
                "Không chạy được thuật toán: " + error.message;
        }

        alert("Lỗi chạy thuật toán: " + error.message);
    }
}

// ======================================================
// HIEN THI KET QUA LICH THI
// ======================================================

function renderResults(failedCourses = []) {
    const result = document.getElementById("result");
    const scheduleResult = document.getElementById("schedule-result");

    if (!result) return;

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
        students.textContent =
            `Số sinh viên: ${schedule.studentCount ?? 0}`;

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

// ======================================================
// HIEN THI BANG THONG KE
// ======================================================

function showStatistics(type) {
    const detail = document.getElementById("statistics-detail");
    const title = document.getElementById("statistics-title");
    const description = document.getElementById("statistics-description");
    const head = document.getElementById("statistics-head");
    const body = document.getElementById("statistics-body");

    if (!detail || !title || !head || !body) {
        alert("Không tìm thấy khu vực bảng thống kê trong index.html.");
        return;
    }

    if (!dataLoaded || !examData) {
        alert("Bà bấm Đọc dữ liệu trước nha!");
        return;
    }

    detail.dataset.type = type;
    detail.classList.remove("hidden");

    head.replaceChildren();
    body.replaceChildren();

    const makeHeader = labels => {
        const tr = document.createElement("tr");

        labels.forEach(label => {
            const th = document.createElement("th");
            th.textContent = label;
            tr.appendChild(th);
        });

        head.appendChild(tr);
    };

    const makeRow = values => {
        const tr = document.createElement("tr");

        values.forEach(value => {
            const td = document.createElement("td");
            td.textContent =
                value === null || value === undefined ? "" : String(value);
            tr.appendChild(td);
        });

        body.appendChild(tr);
    };

    const showEmpty = message => {
        const tr = document.createElement("tr");
        const td = document.createElement("td");

        td.textContent = message;
        td.colSpan = 5;

        tr.appendChild(td);
        body.appendChild(tr);
    };

    // --------------------------------------------------
    // BANG MON HOC
    // --------------------------------------------------

    if (type === "courses") {
        title.textContent = "Danh sách môn thi";

        description.textContent =
            "Danh sách môn học và số sinh viên đăng ký từng môn.";

        makeHeader([
            "STT",
            "Mã môn",
            "Tên môn",
            "Thời lượng (phút)",
            "Số sinh viên đăng ký"
        ]);

        const courses = examData.courseList || [];

        if (courses.length === 0) {
            showEmpty("Không có dữ liệu môn học.");
        } else {
            courses.forEach((course, index) => {
                makeRow([
                    index + 1,
                    course.id ?? course.courseId ?? "",
                    course.name ?? course.courseName ?? "",
                    course.duration ?? "",
                    course.studentCount ?? course.students ?? 0
                ]);
            });
        }
    }

    // --------------------------------------------------
    // BANG PHONG THI
    // --------------------------------------------------

    else if (type === "rooms") {
        title.textContent = "Danh sách phòng thi";

        description.textContent =
            "Thông tin sức chứa và số lịch thi đã xếp cho từng phòng.";

        makeHeader([
            "STT",
            "Mã phòng",
            "Sức chứa",
            "Số lịch đã xếp",
            "Số sinh viên đã xếp"
        ]);

        const rooms = examData.roomList || [];

        const roomStatistics =
            lastScheduleData?.roomStatistics || [];

        const statsByRoom = new Map(
            roomStatistics.map(room => [room.id, room])
        );

        if (rooms.length === 0) {
            showEmpty("Không có dữ liệu phòng thi.");
        } else {
            rooms.forEach((room, index) => {
                const stats = statsByRoom.get(room.id) || {};

                makeRow([
                    index + 1,
                    room.id ?? room.roomId ?? "",
                    room.capacity ?? "",
                    stats.scheduleCount ?? 0,
                    stats.studentCount ?? 0
                ]);
            });
        }
    }

    // --------------------------------------------------
    // BANG CA THI
    // --------------------------------------------------

    else if (type === "slots") {
        title.textContent = "Danh sách ca thi";

        description.textContent =
            "Ngày thi, thời gian bắt đầu, kết thúc và số lịch đã xếp.";

        makeHeader([
            "STT",
            "Mã ca",
            "Ngày thi",
            "Giờ bắt đầu",
            "Giờ kết thúc",
            "Số lịch đã xếp"
        ]);

        const slots = examData.slotList || [];

        const slotStatistics =
            lastScheduleData?.slotStatistics || [];

        const statsBySlot = new Map(
            slotStatistics.map(slot => [slot.id, slot])
        );

        if (slots.length === 0) {
            showEmpty("Không có dữ liệu ca thi.");
        } else {
            slots.forEach((slot, index) => {
                const stats = statsBySlot.get(slot.id) || {};

                makeRow([
                    index + 1,
                    slot.id ?? slot.timeSlotId ?? "",
                    slot.date ?? slot.examDate ?? "",
                    slot.start ?? slot.startTime ?? "",
                    slot.end ?? slot.endTime ?? "",
                    stats.scheduleCount ?? 0
                ]);
            });
        }
    }

    // --------------------------------------------------
    // BANG LICH THI DA TAO
    // --------------------------------------------------

    else if (type === "schedules") {
        title.textContent = "Danh sách lịch thi đã tạo";

        description.textContent =
            "Kết quả xếp lịch của thuật toán được chạy gần nhất.";

        makeHeader([
            "STT",
            "Mã môn",
            "Tên môn",
            "Phòng thi",
            "Ngày thi",
            "Giờ bắt đầu",
            "Giờ kết thúc",
            "Số sinh viên"
        ]);

        if (schedules.length === 0) {
            showEmpty("Chưa có lịch thi. Hãy chạy một thuật toán trước.");
        } else {
            schedules.forEach((schedule, index) => {
                makeRow([
                    index + 1,
                    schedule.courseId ?? "",
                    schedule.courseName ?? "",
                    schedule.roomId ?? "",
                    schedule.date ?? "",
                    schedule.start ?? "",
                    schedule.end ?? "",
                    schedule.studentCount ?? 0
                ]);
            });
        }
    }

    else {
        title.textContent = "Thống kê";
        description.textContent = "Loại thống kê không hợp lệ.";
        showEmpty("Không tìm thấy dữ liệu.");
    }

    // Dua bang vao tam nhin.
    detail.scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
}

// ======================================================
// DONG BANG THONG KE
// ======================================================

function closeStatistics() {
    const detail = document.getElementById("statistics-detail");

    if (detail) {
        detail.classList.add("hidden");
        delete detail.dataset.type;
    }
}

// ======================================================
// TU DONG TAI DU LIEU KHI MO TRANG
// ======================================================

document.addEventListener("DOMContentLoaded", () => {
    loadData();
});