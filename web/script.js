let dataLoaded = false;
let schedules = [];
let examData = null;
let lastScheduleData = null;
let selectedRoomFilter = "all";
let visibleScheduleCount = 5;

const SCHEDULE_PAGE_SIZE = 5;

const algorithmNames = {
    greedy: "Greedy",
    backtracking: "Backtracking",
    dp: "Dynamic Programming"
};

// ======================================================
// GOI API
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
// LAY DANH SACH SINH VIEN
// ======================================================

function getStudentsFromData(data) {
    if (!data) return [];

    if (Array.isArray(data.students)) {
        return data.students;
    }

    if (Array.isArray(data.studentList)) {
        return data.studentList;
    }

    if (Array.isArray(data.student_list)) {
        return data.student_list;
    }

    return [];
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

        const students = getStudentsFromData(data);

        const totalStudents =
            data.totalStudents ??
            data.studentCount ??
            data.total_students ??
            (students.length > 0 ? students.length : "--");

        document.getElementById("students").textContent =
            totalStudents;

        if (message) {
            message.textContent =
                "Đọc dữ liệu thành công!";
        }

        renderResults();

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
        alert(" Đọc dữ liệu trước nha!");
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
            const failedCount =
                (data.failedCourses || []).length;

            message.textContent =
                `Thuật toán ${data.algorithm || algorithmNames[algorithm]} đã chạy xong! ` +
                `Xếp được ${schedules.length} lịch thi. ` +
                `Chưa xếp được: ${failedCount} môn. ` +
                `Thời gian chạy: ${Number(data.runtimeMs || 0).toFixed(3)} ms.`;
        }

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
// HIEN THI THONG TIN SINH VIEN TRONG LICH
// ======================================================

function getScheduleStudents(schedule) {
    return Array.isArray(schedule.students)
        ? schedule.students
        : [];
}

function getScheduleStudentCount(schedule) {
    const students = getScheduleStudents(schedule);

    return Number(
        schedule.studentCount ??
        schedule.student_count ??
        students.length
    );
}

// ======================================================
// TAO CARD LICH THI
// ======================================================

function createScheduleCard(schedule, index) {
    const row = document.createElement("article");
    row.className = "result-row schedule-card";

    const info = document.createElement("div");
    info.className = "schedule-card-info";

    const title = document.createElement("strong");
    title.className = "schedule-card-title";

    title.textContent =
        `${schedule.courseId || ""} - ${schedule.courseName || "Môn thi"}`;

    const details = document.createElement("p");

    details.textContent =
        `Mã lịch: ${schedule.id || index + 1} | ` +
        `Phòng: ${schedule.roomId || "Chưa rõ"} | ` +
        `Ngày: ${schedule.date || "Chưa rõ"} | ` +
        `Giờ: ${schedule.start || "?"} - ${schedule.end || "?"}`;

    const students = getScheduleStudents(schedule);
    const studentCount = getScheduleStudentCount(schedule);

    const countText = document.createElement("p");
    countText.className = "schedule-student-count";
    countText.textContent =
        `Số sinh viên: ${studentCount}`;

    info.append(title, details, countText);
    row.appendChild(info);

    const toggleButton = document.createElement("button");

    toggleButton.type = "button";
    toggleButton.className = "student-toggle-button";
    toggleButton.textContent = "Xem danh sách sinh viên";
    toggleButton.setAttribute("aria-expanded", "false");

    const studentPanel = document.createElement("div");
    studentPanel.className = "schedule-students hidden";

    if (students.length > 0) {
        const table = document.createElement("table");
        table.className = "student-list-table";

        const thead = document.createElement("thead");
        const headerRow = document.createElement("tr");

        ["STT", "Mã sinh viên", "Họ và tên"].forEach(label => {
            const th = document.createElement("th");
            th.textContent = label;
            headerRow.appendChild(th);
        });

        thead.appendChild(headerRow);

        const tbody = document.createElement("tbody");

        students.forEach((student, studentIndex) => {
            const tr = document.createElement("tr");

            const studentId = typeof student === "string"
                ? student
                : (
                    student.studentId ??
                    student.id ??
                    student.student_id ??
                    ""
                );

            const fullName = typeof student === "string"
                ? ""
                : (
                    student.fullName ??
                    student.name ??
                    student.full_name ??
                    ""
                );

            [
                studentIndex + 1,
                studentId,
                fullName
            ].forEach(value => {
                const td = document.createElement("td");
                td.textContent = String(value ?? "");
                tr.appendChild(td);
            });

            tbody.appendChild(tr);
        });

        table.append(thead, tbody);
        studentPanel.appendChild(table);

    } else {
        const empty = document.createElement("p");
        empty.className = "student-list-empty";

        empty.textContent = studentCount > 0
            ? "API mới trả về số lượng sinh viên, chưa có danh sách chi tiết. Cần cập nhật API C++ để trả về mảng students."
            : "Lịch thi này chưa có sinh viên trong dữ liệu trả về.";

        studentPanel.appendChild(empty);
    }

    toggleButton.addEventListener("click", () => {
        const isHidden =
            studentPanel.classList.toggle("hidden");

        toggleButton.textContent = isHidden
            ? "Xem danh sách sinh viên"
            : "Thu gọn danh sách sinh viên";

        toggleButton.setAttribute(
            "aria-expanded",
            String(!isHidden)
        );
    });

    row.append(toggleButton, studentPanel);

    return row;
}

// ======================================================
// LOC LICH THI THEO PHONG VA XEM THEM
// ======================================================

function renderScheduleContainer(container, failedCourses = []) {
    if (!container) return;

    container.replaceChildren();

    const roomIds = [
        ...new Set(
            schedules
                .map(schedule => schedule.roomId)
                .filter(Boolean)
        )
    ].sort((a, b) =>
        String(a).localeCompare(
            String(b),
            "vi",
            { numeric: true }
        )
    );

    const toolbar = document.createElement("div");
    toolbar.className = "schedule-toolbar";

    const label = document.createElement("label");
    label.textContent = "Lọc theo phòng thi: ";

    const select = document.createElement("select");
    select.className = "room-filter-select";

    const allOption = document.createElement("option");
    allOption.value = "all";
    allOption.textContent = "Tất cả phòng";

    select.appendChild(allOption);

    roomIds.forEach(roomId => {
        const option = document.createElement("option");

        option.value = String(roomId);
        option.textContent = `Phòng ${roomId}`;

        select.appendChild(option);
    });

    select.value = roomIds.some(roomId =>
        String(roomId) === selectedRoomFilter
    ) ? selectedRoomFilter : "all";

    label.htmlFor = "room-filter-select";
    select.id = "room-filter-select";

    select.addEventListener("change", () => {
        selectedRoomFilter = select.value;
        visibleScheduleCount = SCHEDULE_PAGE_SIZE;

        renderResults(failedCourses);
    });

    const summary = document.createElement("span");
    summary.className = "schedule-filter-summary";

    toolbar.append(label, select, summary);
    container.appendChild(toolbar);

    const filteredSchedules =
        selectedRoomFilter === "all"
            ? schedules
            : schedules.filter(schedule =>
                String(schedule.roomId) === selectedRoomFilter
            );

    summary.textContent =
        `Hiển thị ${Math.min(
            visibleScheduleCount,
            filteredSchedules.length
        )} / ${filteredSchedules.length} lịch thi`;

    if (filteredSchedules.length === 0) {
        const empty = document.createElement("p");
        empty.className = "empty-state";

        empty.textContent = schedules.length === 0
            ? "Chưa có lịch thi được tạo."
            : "Không có lịch thi trong phòng đã chọn.";

        container.appendChild(empty);

    } else {
        filteredSchedules
            .slice(0, visibleScheduleCount)
            .forEach((schedule, index) => {
                container.appendChild(
                    createScheduleCard(schedule, index)
                );
            });

        if (filteredSchedules.length > visibleScheduleCount) {
            const moreButton = document.createElement("button");

            moreButton.type = "button";
            moreButton.className = "load-more-button";
            moreButton.textContent = "Xem thêm lịch thi";

            moreButton.addEventListener("click", () => {
                visibleScheduleCount += SCHEDULE_PAGE_SIZE;
                renderResults(failedCourses);
            });

            container.appendChild(moreButton);
        }
    }

    if (failedCourses.length > 0) {
        const warning = document.createElement("div");
        warning.className = "result-row warning-row";

        const text = document.createElement("p");

        text.textContent =
            "Các môn chưa xếp được: " +
            failedCourses.join(", ");

        warning.appendChild(text);
        container.appendChild(warning);
    }
}

function renderResults(failedCourses = []) {
    const result = document.getElementById("result");
    const scheduleResult =
        document.getElementById("schedule-result");

    if (result) {
        renderScheduleContainer(result, failedCourses);
    }

    if (scheduleResult && scheduleResult !== result) {
        renderScheduleContainer(
            scheduleResult,
            failedCourses
        );
    }
}

// ======================================================
// THONG KE CHI TIET
// ======================================================

function showStatistics(type) {
    const detail =
        document.getElementById("statistics-detail");

    const title =
        document.getElementById("statistics-title");

    const description =
        document.getElementById("statistics-description");

    const head =
        document.getElementById("statistics-head");

    const body =
        document.getElementById("statistics-body");

    if (!detail || !title || !head || !body) {
        alert("Không tìm thấy khu vực bảng thống kê.");
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

    function makeHeader(labels) {
        const tr = document.createElement("tr");

        labels.forEach(label => {
            const th = document.createElement("th");
            th.textContent = label;
            tr.appendChild(th);
        });

        head.appendChild(tr);
    }

    function makeRow(values) {
        const tr = document.createElement("tr");

        values.forEach(value => {
            const td = document.createElement("td");

            td.textContent =
                value === null || value === undefined
                    ? ""
                    : String(value);

            tr.appendChild(td);
        });

        body.appendChild(tr);
    }

    function showEmpty(message) {
        const tr = document.createElement("tr");
        const td = document.createElement("td");

        td.textContent = message;
        td.colSpan = 8;

        tr.appendChild(td);
        body.appendChild(tr);
    }

    // MON HOC
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

    // PHONG THI
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
            roomStatistics.map(room => [
                room.id ?? room.roomId,
                room
            ])
        );

        if (rooms.length === 0) {
            showEmpty("Không có dữ liệu phòng thi.");
        } else {
            rooms.forEach((room, index) => {
                const stats = statsByRoom.get(
                    room.id ?? room.roomId
                ) || {};

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

    // CA THI
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
            slotStatistics.map(slot => [
                slot.id ?? slot.timeSlotId,
                slot
            ])
        );

        if (slots.length === 0) {
            showEmpty("Không có dữ liệu ca thi.");
        } else {
            slots.forEach((slot, index) => {
                const stats = statsBySlot.get(
                    slot.id ?? slot.timeSlotId
                ) || {};

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

    // LICH THI
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
            showEmpty(
                "Chưa có lịch thi. Hãy chạy một thuật toán trước."
            );
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

    // TONG SO SINH VIEN
    else if (type === "students") {
        title.textContent = "Danh sách sinh viên";

        description.textContent =
            "Danh sách sinh viên trong dữ liệu hệ thống.";

        makeHeader([
            "STT",
            "Mã sinh viên",
            "Họ và tên"
        ]);

        const students = getStudentsFromData(examData);

        if (students.length === 0) {
            showEmpty(
                "API chưa trả về danh sách sinh viên. Cần bổ sung dữ liệu sinh viên trong API C++."
            );
        } else {
            students.forEach((student, index) => {
                makeRow([
                    index + 1,
                    student.studentId ??
                        student.id ??
                        student.student_id ??
                        "",
                    student.fullName ??
                        student.name ??
                        student.full_name ??
                        ""
                ]);
            });
        }
    }

    else {
        title.textContent = "Thống kê";
        description.textContent =
            "Loại thống kê không hợp lệ.";

        showEmpty("Không tìm thấy dữ liệu.");
    }

    detail.scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
}

// ======================================================
// DONG BANG THONG KE
// ======================================================

function closeStatistics() {
    const detail =
        document.getElementById("statistics-detail");

    if (detail) {
        detail.classList.add("hidden");
        delete detail.dataset.type;
    }
}

// ======================================================
// TU DONG DOC DU LIEU KHI MO TRANG
// ======================================================

