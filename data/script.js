// ==================== WEBSOCKET ====================

// Dùng host (có cả port) để WebSocket luôn đúng với port 8080
var gateway = `ws://${window.location.host}/ws`;
var websocket = null;

window.addEventListener('load', function () {
    initWebSocket();
    initGauges();
    loadCoreIOTConfig();
    pollSensors();
});

function initWebSocket() {
    console.log('🔌 Mở kết nối WebSocket...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('✅ WebSocket đã kết nối');
}

function onClose(event) {
    console.log('⚠️ WebSocket đóng, thử lại sau 2 giây...');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log("📩 Nhận:", event.data);
    try {
        var data = JSON.parse(event.data);
        
        // Cập nhật dữ liệu hiện tại
        if (data.temperature !== undefined && window.gaugeTemp) {
            window.gaugeTemp.refresh(data.temperature);
        }
        if (data.humidity !== undefined && window.gaugeHumi) {
            window.gaugeHumi.refresh(data.humidity);
        }
        
        // Cập nhật dữ liệu dự đoán
        if (data.predicted_temp !== undefined && data.predicted_temp !== null && window.gaugePredictTemp) {
            window.gaugePredictTemp.refresh(data.predicted_temp);
        }
        if (data.predicted_humi !== undefined && data.predicted_humi !== null && window.gaugePredictHumi) {
            window.gaugePredictHumi.refresh(data.predicted_humi);
        }
        if (data.accuracy !== undefined && data.accuracy !== null && window.gaugeAccuracy) {
            window.gaugeAccuracy.refresh(data.accuracy);
        }
    } catch (e) {
        console.warn("⚠️ Dữ liệu nhận được không phải JSON hợp lệ:", event.data);
    }
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        var payload = (typeof data === "string") ? data : JSON.stringify(data);
        websocket.send(payload);
        console.log("📤 Gửi:", payload);
    } else {
        console.warn("⚠️ WebSocket chưa sẵn sàng, không thể gửi dữ liệu!");
    }
}

// ==================== UI NAVIGATION ====================
let relayList = [];
let deleteTarget = null;

function showSection(id, event) {
    // Ẩn tất cả sections
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');

    // Hiện section được chọn
    document.getElementById(id).style.display = (id === 'settings') ? 'flex' : 'block';

    // Active menu
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Tải lại cấu hình khi mở phần Settings
    if (id === 'settings') {
        loadCoreIOTConfig();
    }
}

// ==================== HOME GAUGES ====================
function initGauges() {
    // Nhiệt độ hiện tại
    window.gaugeTemp = new JustGage({
        id: "gauge_temp",
        value: 100,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#00BCD4", "#4CAF50", "#FFC107", "#F44336"],
        counter: false,
        startAnimationTime: 0,
        startAnimationType: "linear",
        refreshAnimationTime: 1000
    });

    // Độ ẩm hiện tại
    window.gaugeHumi = new JustGage({
        id: "gauge_humi",
        value: 100,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#42A5F5", "#00BCD4", "#0288D1"],
        counter: false,
        startAnimationTime: 0,
        startAnimationType: "linear",
        refreshAnimationTime: 1000
    });

    // Dự báo nhiệt độ
    window.gaugePredictTemp = new JustGage({
        id: "gauge_predict_temp",
        value: 100,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#FA709A", "#FEE140", "#FF9A9E"],
        counter: false,
        startAnimationTime: 0,
        startAnimationType: "linear",
        refreshAnimationTime: 1000
    });

    // Dự báo độ ẩm
    window.gaugePredictHumi = new JustGage({
        id: "gauge_predict_humi",
        value: 100,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#30CFD0", "#330867", "#A8EDEA"],
        counter: false,
        startAnimationTime: 0,
        startAnimationType: "linear",
        refreshAnimationTime: 1000
    });

    // Độ chính xác
    window.gaugeAccuracy = new JustGage({
        id: "gauge_accuracy",
        value: 100,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#A8EDEA", "#FED6E3", "#96E6A1"],
        counter: false,
        startAnimationTime: 0,
        startAnimationType: "linear",
        refreshAnimationTime: 1000
    });
}

// ==================== DEVICE FUNCTIONS ====================
function openAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'flex';
}

function closeAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'none';
}

function saveRelay() {
    const name = document.getElementById('relayName').value.trim();
    const gpio = document.getElementById('relayGPIO').value.trim();

    if (!name || !gpio) {
        alert("⚠️ Vui lòng điền đầy đủ tên và GPIO!");
        return;
    }

    relayList.push({ id: Date.now(), name, gpio, state: false });
    renderRelays();
    closeAddRelayDialog();
}

function renderRelays() {
    const container = document.getElementById('relayContainer');
    container.innerHTML = "";

    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';
        card.innerHTML = `
            <i class="fa-solid fa-bolt device-icon"></i>
            <h3>${r.name}</h3>
            <p>GPIO: ${r.gpio}</p>
            <button class="toggle-btn ${r.state ? 'on' : ''}" onclick="toggleRelay(${r.id})">
                ${r.state ? 'ON' : 'OFF'}
            </button>
            <i class="fa-solid fa-trash delete-icon" onclick="showDeleteDialog(${r.id})"></i>
        `;
        container.appendChild(card);
    });
}

function toggleRelay(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay) {
        relay.state = !relay.state;

        Send_Data({
            page: "device",
            value: {
                name: relay.name,
                status: relay.state ? "ON" : "OFF",
                gpio: relay.gpio
            }
        });

        renderRelays();
    }
}

function showDeleteDialog(id) {
    deleteTarget = id;
    document.getElementById('confirmDeleteDialog').style.display = 'flex';
}

function closeConfirmDelete() {
    document.getElementById('confirmDeleteDialog').style.display = 'none';
}

function confirmDelete() {
    relayList = relayList.filter(r => r.id !== deleteTarget);
    renderRelays();
    closeConfirmDelete();
}

// ==================== COREIOT CONFIG API ====================
async function loadCoreIOTConfig() {
    try {
        const response = await fetch('/api/coreiot/config');
        if (!response.ok) {
            console.warn("⚠️ Không tải được cấu hình CoreIOT!");
            return;
        }

        const data = await response.json();

        if (data.server)    document.getElementById('server').value = data.server;
        if (data.port)      document.getElementById('port').value = data.port;
        if (data.client_id) document.getElementById('client_id').value = data.client_id;
        if (data.username)  document.getElementById('mqtt_username').value = data.username;

        const mqttPassInput = document.getElementById('mqtt_password');
        mqttPassInput.value = "";
        mqttPassInput.placeholder = data.password_set
            ? "Mật khẩu đã lưu (để trống = giữ nguyên)"
            : "Password (MQTT)";

        console.log("✅ Đã tải cấu hình CoreIOT");
    } catch (error) {
        console.error("❌ Lỗi tải cấu hình:", error);
    }
}

document.getElementById("settingsForm").addEventListener("submit", async function (e) {
    e.preventDefault();

    const server        = document.getElementById("server").value.trim();
    const portValue     = document.getElementById("port").value.trim();
    const client_id     = document.getElementById("client_id").value.trim();
    const mqtt_username = document.getElementById("mqtt_username").value.trim();
    const mqtt_password = document.getElementById("mqtt_password").value.trim();

    const port = parseInt(portValue, 10);
    if (!Number.isInteger(port) || port <= 0 || port > 65535) {
        alert("⚠️ Port không hợp lệ! (1–65535)");
        return;
    }

    if (!server || !client_id || !mqtt_username) {
        alert("⚠️ Vui lòng điền đủ: Server, Client ID và Username!");
        return;
    }

    const config = {
        server    : server,
        port      : port,
        client_id : client_id,
        username  : mqtt_username,
        password  : mqtt_password || "***"
    };

    try {
        const response = await fetch('/api/coreiot/config', {
            method : 'POST',
            headers: { 'Content-Type': 'application/json' },
            body   : JSON.stringify(config)
        });

        const result = await response.json();

        if (result.success) {
            alert("✅ Đã lưu cấu hình! MQTT sẽ kết nối lại.");
            setTimeout(loadCoreIOTConfig, 500);
        } else {
            alert("❌ Lỗi: " + (result.message || "Không rõ nguyên nhân"));
        }
    } catch (error) {
        console.error("❌ Lỗi gửi cấu hình:", error);
        alert("❌ Không thể kết nối đến ESP32!");
    }
});

// ==================== SENSOR POLLING ====================
async function pollSensors() {
    try {
        const res = await fetch('/sensor');
        if (!res.ok) return;

        const data = await res.json();
        if (data.error) return;

        // Cập nhật dữ liệu hiện tại
        if (window.gaugeTemp) window.gaugeTemp.refresh(data.temperature ?? 0);
        if (window.gaugeHumi) window.gaugeHumi.refresh(data.humidity ?? 0);

        // Cập nhật dữ liệu dự đoán (nếu có)
        if (data.predicted_temp !== null && window.gaugePredictTemp) {
            window.gaugePredictTemp.refresh(data.predicted_temp ?? 0);
        }
        if (data.predicted_humi !== null && window.gaugePredictHumi) {
            window.gaugePredictHumi.refresh(data.predicted_humi ?? 0);
        }
        if (data.accuracy !== null && window.gaugeAccuracy) {
            window.gaugeAccuracy.refresh(data.accuracy ?? 0);
        }

    } catch (err) {
        console.warn('⚠️ Lỗi tải dữ liệu cảm biến', err);
    }
}

setInterval(pollSensors, 5000);