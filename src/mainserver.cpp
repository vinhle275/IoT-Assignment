#include "mainserver.h"
#include "global.h" // Include thêm file này để lấy định nghĩa các struct như MqttLocalConfig_t
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>

// Khai báo extern các queue và semaphore từ main.cpp
extern QueueHandle_t sensorQueue;
extern QueueHandle_t weatherQueue;
extern QueueHandle_t wifiConfigQueue;
extern QueueHandle_t localMqttQueue; // Thêm localMqttQueue để đọc/ghi cấu hình
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// Biến trạng thái
bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;
WebServer server(80);
unsigned long connect_start_ms = 0;
bool connecting = false;

// --- Giao diện Dashboard Modern (Giữ nguyên) ---
String mainPage() {
  float temperature = 0.0;
  float humidity = 0.0;
  String weatherLabel = "Updating..."; 
  SensorData_t rData;
  WeatherData_t rWeather;
  if (sensorQueue != NULL && xQueuePeek(sensorQueue, &rData, 0) == pdPASS) {
      temperature = rData.temperature;
      humidity = rData.humidity;
  }
  if (weatherQueue != NULL && xQueuePeek(weatherQueue, &rWeather, 0) == pdPASS) {
      weatherLabel = String(rWeather.label);
  }
  String l1 = led1_state ? "ON" : "OFF";
  String l2 = led2_state ? "ON" : "OFF";
  return R"rawliteral(<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AsynapRous Hub</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    :root { 
      --bg: #f8fafc; --surface: #ffffff; --primary: #3b82f6; --secondary: #10b981;
      --text: #0f172a; --muted: #64748b; --border: #e2e8f0; --shadow: rgba(0,0,0,0.05);
    }
    [data-theme="dark"] {
      --bg: #0f172a; --surface: #1e293b; --primary: #60a5fa; --secondary: #34d399;
      --text: #f8fafc; --muted: #94a3b8; --border: #334155; --shadow: rgba(0,0,0,0.3);
    }
    * { box-sizing: border-box; font-family: 'Outfit', sans-serif; transition: 0.3s; }
    body { margin: 0; background: var(--bg); color: var(--text); padding: 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    .header { width: 100%; max-width: 900px; display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; }
    .header h2 { font-weight: 700; margin: 0; }
    .header-actions { display: flex; gap: 10px; }
    .theme-btn { 
      background: var(--surface); border: 1px solid var(--border); width: 42px; height: 42px; 
      border-radius: 12px; cursor: pointer; display: flex; align-items: center; justify-content: center; font-size: 20px;
    }
    .nav-btn { 
      background: var(--surface); color: var(--text); border: 1px solid var(--border); 
      padding: 10px 18px; border-radius: 12px; text-decoration: none; font-size: 13px; font-weight: 600;
    }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; width: 100%; max-width: 900px; margin-bottom: 25px; }
    .card { background: var(--surface); border-radius: 24px; padding: 25px; border: 1px solid var(--border); box-shadow: 0 4px 6px var(--shadow); }
    .value { font-size: 40px; font-weight: 700; margin: 10px 0; }
    .label { color: var(--muted); font-size: 12px; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; }
    .btn { width: 100%; padding: 14px; border: 1px solid var(--border); border-radius: 14px; font-weight: 700; cursor: pointer; margin-top: 10px; background: var(--bg); color: var(--text); }
    .btn.on { background: var(--secondary); color: #fff; border-color: var(--secondary); }
    .chart-box { background: var(--surface); padding: 25px; border-radius: 24px; width: 100%; max-width: 900px; border: 1px solid var(--border); margin-bottom: 25px; box-shadow: 0 4px 6px var(--shadow); }
    canvas { width: 100%; height: 240px; display: block; }
  </style>
</head>
<body>
  <div class="header">
    <h2>ASYNAPROUS HUB</h2>
    <div class="header-actions">
      <button class="theme-btn" onclick="toggleTheme()" id="themeIcon">🌙</button>
      <a href="/settings" class="nav-btn">WI-FI & MQTT</a>
    </div>
  </div>
  <div class="grid">
    <div class="card"><div class="label">Nhiệt độ</div><div class="value"><span id="t">)rawliteral" + String(temperature) + R"rawliteral(</span>°C</div></div>
    <div class="card"><div class="label">Độ ẩm</div><div class="value"><span id="h">)rawliteral" + String(humidity) + R"rawliteral(</span>%</div></div>
    <div class="card"><div class="label">Dự báo AI</div><div class="value" style="font-size: 20px;"><span id="p">)rawliteral" + weatherLabel + R"rawliteral(</span></div></div>
    <div class="card">
      <div class="label">Thiết bị</div>
      <button id="b1" class="btn )rawliteral" + String(led1_state ? "on" : "") + R"rawliteral(" onclick="tk(1)">LED: <span id="l1">)rawliteral" + l1 + R"rawliteral(</span></button>
      <button id="b2" class="btn )rawliteral" + String(led2_state ? "on" : "") + R"rawliteral(" onclick="tk(2)">RGB: <span id="l2">)rawliteral" + l2 + R"rawliteral(</span></button>
    </div>
  </div>
  <div class="chart-box"><div class="label" style="margin-bottom:15px">Temperature History (60s)</div><canvas id="ct"></canvas></div>
  <div class="chart-box"><div class="label" style="margin-bottom:15px">Humidity History (60s)</div><canvas id="ch"></canvas></div>

  <script>
    let dataT = [], dataH = [], labels = [];
    const MAX_POINTS = 20;

    function toggleTheme() {
      const current = document.documentElement.getAttribute('data-theme');
      const target = current === 'dark' ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', target);
      document.getElementById('themeIcon').innerText = target === 'dark' ? '☀️' : '🌙';
      localStorage.setItem('theme', target);
      refreshCharts();
    }

    const savedTheme = localStorage.getItem('theme') || 'light';
    document.documentElement.setAttribute('data-theme', savedTheme);
    document.getElementById('themeIcon').innerText = savedTheme === 'dark' ? '☀️' : '🌙';

    function tk(id) {
      fetch('/toggle?led=' + id + '&_t=' + Date.now()).then(r => r.json()).then(j => {
        document.getElementById('l1').innerText = j.led1; document.getElementById('l2').innerText = j.led2;
        document.getElementById('b1').className = 'btn ' + (j.led1 === "ON" ? "on" : "");
        document.getElementById('b2').className = 'btn ' + (j.led2 === "ON" ? "on" : "");
      });
    }

    function drawChart(canId, data, timeLabels, color, minV, maxV, unit) {
      const can = document.getElementById(canId);
      const ctx = can.getContext('2d');
      const dpr = window.devicePixelRatio || 1;
      can.width = can.offsetWidth * dpr; can.height = can.offsetHeight * dpr;
      ctx.scale(dpr, dpr);
      const w = can.offsetWidth, h = can.offsetHeight;
      const padL = 50, padB = 40, padT = 20, padR = 20;
      const gH = h - padB - padT, gW = w - padL - padR;
      const isDark = document.documentElement.getAttribute('data-theme') === 'dark';

      ctx.clearRect(0, 0, w, h);
      ctx.strokeStyle = isDark ? "rgba(255,255,255,0.05)" : "rgba(0,0,0,0.05)";
      ctx.fillStyle = "#94a3b8"; ctx.font = "600 11px Outfit";

      for(let i=0; i<=4; i++) {
        let val = minV + (maxV - minV) * (i/4);
        let y = (h - padB) - (i/4) * gH;
        ctx.fillText(val.toFixed(0) + unit, 10, y + 4);
        ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke();
      }

      if (data.length < 2) return;
      ctx.textAlign = "center";
      data.forEach((v, i) => {
        if (i % 5 === 0 || i === data.length - 1) {
          const x = padL + (i / (MAX_POINTS - 1)) * gW;
          ctx.fillText(timeLabels[i] || "", x, h - 10);
        }
      });

      const grad = ctx.createLinearGradient(0, padT, 0, h - padB);
      grad.addColorStop(0, color + "33"); grad.addColorStop(1, color + "00");
      ctx.fillStyle = grad; ctx.beginPath();
      data.forEach((v, i) => {
        const x = padL + (i / (MAX_POINTS - 1)) * gW;
        const y = (h - padB) - ((v - minV) / (maxV - minV)) * gH;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      });
      ctx.lineTo(padL + ((data.length-1) / (MAX_POINTS - 1)) * gW, h - padB);
      ctx.lineTo(padL, h - padB); ctx.fill();

      ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 4; ctx.lineJoin = "round";
      if(isDark) { ctx.shadowBlur = 8; ctx.shadowColor = color; }
      data.forEach((v, i) => {
        const x = padL + (i / (MAX_POINTS - 1)) * gW;
        const y = (h - padB) - ((v - minV) / (maxV - minV)) * gH;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      });
      ctx.stroke(); ctx.shadowBlur = 0;
    }

    function refreshCharts() {
      drawChart('ct', dataT, labels, '#3b82f6', 0, 50, '°'); 
      drawChart('ch', dataH, labels, '#10b981', 0, 100, '%');
    }

    function update() {
      fetch('/sensors?_t=' + Date.now()).then(r => r.json()).then(d => {
        document.getElementById('t').innerText = d.temp.toFixed(1);
        document.getElementById('h').innerText = d.hum.toFixed(1);
        if(d.prediction) document.getElementById('p').innerText = d.prediction;
        const now = new Date();
        const timeStr = now.getHours().toString().padStart(2,'0') + ":" + now.getMinutes().toString().padStart(2,'0') + ":" + now.getSeconds().toString().padStart(2,'0');
        dataT.push(d.temp); dataH.push(d.hum); labels.push(timeStr);
        if (dataT.length > MAX_POINTS) { dataT.shift(); dataH.shift(); labels.shift(); }
        refreshCharts();
      });
    }

    setInterval(update, 3000);
    window.addEventListener('resize', refreshCharts);
  </script>
</body>
</html>)rawliteral";
}

// --- Giao diện Wi-Fi & MQTT Settings (Đã cập nhật để truyền Data Mặc Định) ---
String settingsPage(String devId, String mqttPass, String tinySrv, int port) {
  return R"rawliteral(<!DOCTYPE html> <html lang="vi"> <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wi-Fi & MQTT Config</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    body { background: #f8fafc; color: #0f172a; font-family: 'Outfit'; display: flex; flex-direction: column; align-items: center; min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box;}
    .card { background: #fff; padding: 35px; border-radius: 24px; width: 100%; max-width: 400px; box-shadow: 0 10px 15px rgba(0,0,0,0.05); margin-bottom: 25px;}
    h2 { text-align: center; margin-bottom: 25px; font-size: 22px; }
    select, input { width: 100%; padding: 12px; border: 1px solid #e2e8f0; border-radius: 12px; margin-bottom: 15px; outline: none; box-sizing: border-box; font-family: 'Outfit'; }
    label { font-weight: 600; font-size: 14px; margin-bottom: 6px; display: block; color: #475569; }
    .btn-scan { background: #f1f5f9; color: #3b82f6; border: 1px solid #e2e8f0; padding: 10px; border-radius: 12px; cursor: pointer; width: 100%; margin-bottom: 15px; font-weight: 600; }
    .btn-submit { width: 100%; padding: 15px; border: none; border-radius: 14px; background: #3b82f6; color: #fff; font-weight: 700; cursor: pointer; transition: 0.3s; }
    .btn-submit:hover { background: #2563eb; }
    .btn-mqtt { background: #10b981; }
    .btn-mqtt:hover { background: #059669; }
    .back-btn { display:block; text-align:center; color:#64748b; text-decoration:none; font-weight: 600; margin-bottom: 20px;}
  </style>
</head>
<body>
  <div class="card">
    <h2>🌐 Wi-Fi Setup</h2>
    <button class="btn-scan" onclick="scan()" id="sb">🔍 SCAN NETWORKS</button>
    <select id="sl" onchange="document.getElementById('ss').value=this.value"><option value="">-- Select network --</option></select>
    <form id="wf">
      <input type="text" id="ss" placeholder="SSID" required>
      <input type="password" id="ps" placeholder="Password">
      <button type="submit" class="btn-submit">CONNECT WI-FI</button>
    </form>
    <div id="st" style="margin-top:15px; text-align:center; font-weight:600;"></div>
  </div>

  <div class="card">
    <h2>⚙️ Local MQTT Config</h2>
    <form id="mqttForm">
      <label>Device ID</label>
      <input type="text" id="dev_id" value=")rawliteral" + devId + R"rawliteral(" required>

      <label>MQTT Password</label>
      <input type="text" id="mqtt_pass" value=")rawliteral" + mqttPass + R"rawliteral(">

      <label>Tiny Server IP</label>
      <input type="text" id="tiny_srv" value=")rawliteral" + tinySrv + R"rawliteral(" required>

      <label>Port</label>
      <input type="number" id="port" value=")rawliteral" + String(port) + R"rawliteral(" required>

      <button type="submit" class="btn-submit btn-mqtt">SAVE MQTT CONFIG</button>
    </form>
    <div id="mqtt_st" style="margin-top:15px; text-align:center; color:#10b981; font-weight:600;"></div>
  </div>

  <a href="/" class="back-btn">⬅ Back to Dashboard</a>

  <script>
    function scan() {
      const b=document.getElementById('sb'); b.innerText="SCANNING...";
      fetch('/scan').then(r=>r.json()).then(data=>{
        const s=document.getElementById('sl'); s.innerHTML='<option value="">-- Select network --</option>';
        data.forEach(n=>{ const o=document.createElement('option'); o.value=n.ssid; o.innerHTML=n.ssid+' ('+n.rssi+' dBm)'; s.appendChild(o); });
        b.innerText="🔍 SCAN AGAIN";
      });
    }

    document.getElementById('wf').onsubmit=e=>{
      e.preventDefault();
      fetch(`/connect?ssid=${encodeURIComponent(document.getElementById('ss').value)}&pass=${encodeURIComponent(document.getElementById('ps').value)}`)
      .then(r=>r.text()).then(m=>{ document.getElementById('st').innerText="Connecting..."; setTimeout(()=>window.location.href="/", 2000); });
    };

    document.getElementById('mqttForm').onsubmit=e=>{
      e.preventDefault();
      let d = encodeURIComponent(document.getElementById('dev_id').value);
      let p = encodeURIComponent(document.getElementById('mqtt_pass').value);
      let s = encodeURIComponent(document.getElementById('tiny_srv').value);
      let pt = encodeURIComponent(document.getElementById('port').value);

      document.getElementById('mqtt_st').innerText="Đang lưu...";
      fetch(`/update_mqtt?dev_id=${d}&pass=${p}&server=${s}&port=${pt}`)
      .then(r=>r.text()).then(m=>{
         document.getElementById('mqtt_st').innerText="✅ Đã cập nhật thành công!";
         setTimeout(()=>document.getElementById('mqtt_st').innerText="", 3000);
      });
    };
  </script>
</body>
</html>)rawliteral";
}

// --- Handlers ---
void handleRoot() { 
  server.send(200, "text/html", mainPage()); 
}

void handleToggle() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  int led = server.arg("led").toInt();

  if (led == 1) {
    led1_state = !led1_state;
    // Điều khiển LED đơn (giữ nguyên logic cũ nếu cần)
    if (led1_state) {
      if (uxSemaphoreGetCount(xSemaphoreLedControl) == 0) xSemaphoreGive(xSemaphoreLedControl);
    } else {
      xSemaphoreTake(xSemaphoreLedControl, 0);
    }
  } 
  else if (led == 2) { // Đây là phần điều khiển NeoLED
    led2_state = !led2_state;
    if (led2_state) {
      // Nếu bật: Give semaphore để cho phép NeoLED sáng
      if (uxSemaphoreGetCount(xSemaphoreNeoControl) == 0) {
        xSemaphoreGive(xSemaphoreNeoControl);
      }
    } else {
      // Nếu tắt: Take semaphore để dừng NeoLED
      xSemaphoreTake(xSemaphoreNeoControl, 0);
    }
  }

  server.send(200, "application/json", "{\"led1\":\"" + String(led1_state ? "ON" : "OFF") + "\",\"led2\":\"" + String(led2_state ? "ON" : "OFF") + "\"}");
}

void handleSensors() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  float t = 0.0, h = 0.0; SensorData_t sd; WeatherData_t wd;
  if (sensorQueue && xQueuePeek(sensorQueue, &sd, 0) == pdPASS) { t = sd.temperature; h = sd.humidity; }
  String p = (weatherQueue && xQueuePeek(weatherQueue, &wd, 0) == pdPASS) ? String(wd.label) : "Updating...";
  server.send(200, "application/json", "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + ",\"prediction\":\"" + p + "\"}");
}

void handleWifiScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSettings() {
  MqttLocalConfig_t currentLoc;
  
  // Lấy giá trị hiện hành từ MQTT Queue để điền sẵn vào HTML Web
  if (localMqttQueue != NULL && xQueuePeek(localMqttQueue, &currentLoc, 0) == pdPASS) {
      // Đã lấy thành công cấu hình
  } else {
      // Trường hợp queue trống, cấp giá trị hiển thị mặc định
      strcpy(currentLoc.device_id, "ESP32_001");
      strcpy(currentLoc.mqtt_pass, "1234567890");
      strcpy(currentLoc.tiny_server, "192.168.1.3");
      currentLoc.port = 1883;
  }
  
  server.send(200, "text/html", settingsPage(String(currentLoc.device_id), String(currentLoc.mqtt_pass), String(currentLoc.tiny_server), currentLoc.port));
}

// Xử lý Route Mới Cập nhật cấu hình Local MQTT do người dùng bấm gửi
void handleUpdateMqtt() {
  MqttLocalConfig_t newLoc;

  String dev_id = server.arg("dev_id");
  String pass = server.arg("pass");
  String srv = server.arg("server");
  int port = server.arg("port").toInt();

  // Đổ string vừa đọc được từ form vào biến cấu hình với giới hạn buffer an toàn
  strncpy(newLoc.device_id, dev_id.c_str(), sizeof(newLoc.device_id) - 1);
  newLoc.device_id[sizeof(newLoc.device_id) - 1] = '\0';

  strncpy(newLoc.mqtt_pass, pass.c_str(), sizeof(newLoc.mqtt_pass) - 1);
  newLoc.mqtt_pass[sizeof(newLoc.mqtt_pass) - 1] = '\0';

  strncpy(newLoc.tiny_server, srv.c_str(), sizeof(newLoc.tiny_server) - 1);
  newLoc.tiny_server[sizeof(newLoc.tiny_server) - 1] = '\0';

  newLoc.port = port;

  // Ghi đè vào Queue. Từ đây Task CoreIOT sẽ tự động ngắt kết nối cũ và apply cấu hình mới ngay lập tức
  if (localMqttQueue != NULL) {
      xQueueOverwrite(localMqttQueue, &newLoc);
  }

  server.send(200, "text/plain", "OK");
}

void handleConnect() {
  WifiConfig_t conf; memset(&conf, 0, sizeof(conf));
  strncpy(conf.ssid, server.arg("ssid").c_str(), 31);
  strncpy(conf.password, server.arg("pass").c_str(), 63);
  if (wifiConfigQueue) xQueueOverwrite(wifiConfigQueue, &conf);
  server.send(200, "text/plain", "OK");
  connecting = true; connect_start_ms = millis();
  WiFi.mode(WIFI_STA); WiFi.begin(conf.ssid, conf.password);
}

// --- WiFi & Task Management ---
void startAP() {
  WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASS);
  isAPMode = true; connecting = false;
}

void main_server_task(void *pvParameters) {
  pinMode(BOOT_PIN, INPUT_PULLUP);
  startAP(); 

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/sensors", handleSensors);
  server.on("/settings", handleSettings);
  server.on("/scan", handleWifiScan);
  server.on("/connect", handleConnect);
  
  // Khai báo đường dẫn mới để lắng nghe API cập nhật MQTT
  server.on("/update_mqtt", handleUpdateMqtt);

  server.begin(); 

  while (1) {
    server.handleClient();
    if (digitalRead(BOOT_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(100));
      if (digitalRead(BOOT_PIN) == LOW && !isAPMode) startAP();
    }
    if (connecting) {
      if (WiFi.status() == WL_CONNECTED) {
        if(xBinarySemaphoreInternet) xSemaphoreGive(xBinarySemaphoreInternet);
        isAPMode = false; connecting = false;
      } else if (millis() - connect_start_ms > 15000) {
        startAP(); connecting = false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}