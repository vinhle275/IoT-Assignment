#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>

// LED states removed as per request
bool isAPMode = true;

WebServer server(80);

unsigned long connect_start_ms = 0;
bool connecting = false;

String mainPage()
{
  float temperature = 0.0;
  float humidity = 0.0;
  String weatherLabel = "Calculating..."; // Biến cục bộ để hiển thị tạm thời
  
  SensorData_t receivedData;
  WeatherData_t receivedWeather;

  // Lấy dữ liệu nhiệt ẩm
  if (sensorQueue != NULL && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
      temperature = receivedData.temperature;
      humidity = receivedData.humidity;
  }
  
  // Lấy dữ liệu dự đoán thời tiết từ Queue
  if (weatherQueue != NULL && xQueuePeek(weatherQueue, &receivedWeather, 0) == pdPASS) {
      weatherLabel = String(receivedWeather.label);
  }

  return R"rawliteral(<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #0f172a; --surface: #1e293b; --primary: #38bdf8;
      --text: #f8fafc; --text-muted: #94a3b8; --card-radius: 20px;
    }
    * { box-sizing: border-box; font-family: 'Outfit', sans-serif; }
    body { margin: 0; background-color: var(--bg); color: var(--text); padding: 20px; min-height: 100vh; display: flex; flex-direction: column; align-items: center; }
    .header { width: 100%; max-width: 1000px; display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; }
    .header h1 { margin: 0; font-size: 28px; font-weight: 700; background: linear-gradient(to right, #38bdf8, #818cf8); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .nav-btn { background: var(--surface); color: var(--text); border: 1px solid #334155; padding: 10px 20px; border-radius: 12px; cursor: pointer; text-decoration: none; font-weight: 600; transition: all 0.3s; }
    .nav-btn:hover { background: #334155; transform: translateY(-2px); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; width: 100%; max-width: 1000px; margin-bottom: 20px; }
    .card { background: var(--surface); border-radius: var(--card-radius); padding: 24px; box-shadow: 0 10px 30px -10px rgba(0,0,0,0.5); border: 1px solid #334155; transition: transform 0.3s; position: relative; overflow: hidden;}
    .card:hover { transform: translateY(-5px); }
    .card::before { content: ''; position: absolute; top: 0; left: 0; right: 0; height: 4px; background: linear-gradient(90deg, #38bdf8, #818cf8); opacity: 0; transition: opacity 0.3s;}
    .card:hover::before { opacity: 1; }
    .value { font-size: 48px; font-weight: 700; margin: 10px 0; }
    .label { color: var(--text-muted); font-size: 16px; font-weight: 600; text-transform: uppercase; letter-spacing: 1px; }
    .btn-group { display: flex; gap: 15px; margin-top: 15px; }
    .btn { flex: 1; padding: 14px; border: none; border-radius: 12px; font-size: 16px; font-weight: 600; cursor: pointer; transition: all 0.3s; background: #334155; color: white;}
    .btn.on { background: linear-gradient(135deg, #10b981, #059669); color: white; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4);}
    .btn:hover { filter: brightness(1.1); transform: scale(1.02); }
    .chart-container { background: var(--surface); padding: 20px; border-radius: var(--card-radius); width: 100%; max-width: 1000px; border: 1px solid #334155; box-shadow: 0 10px 30px -10px rgba(0,0,0,0.5); margin-bottom: 20px;}
    canvas { width: 100% !important; max-height: 400px; }
  </style>
</head>
<body>
  <div class="header">
    <h1>🚀 ESP32 IoT Hub</h1>
    <a href="/settings" class="nav-btn">⚙️ Cài đặt & Wi-Fi</a>
  </div>
  
  <div class="grid">
    <div class="card">
      <div class="label">Nhiệt độ</div>
      <div class="value"><span id="temp">)rawliteral" + String(temperature) + R"rawliteral(</span><span style="font-size:24px;color:var(--text-muted)">°C</span></div>
    </div>
    <div class="card">
      <div class="label">Độ ẩm</div>
      <div class="value"><span id="hum">)rawliteral" + String(humidity) + R"rawliteral(</span><span style="font-size:24px;color:var(--text-muted)">%</span></div>
    </div>
    <div class="card">
      <div class="label">AI Dự đoán Thời tiết</div>
      <div class="value" style="font-size: 36px;"><span id="prediction">)rawliteral" + weatherLabel + R"rawliteral(</span></div>
    </div>
  </div>

  <div class="chart-container">
    <div class="label" style="margin-bottom: 15px;">Biểu đồ thời gian thực</div>
    <canvas id="myChart"></canvas>
  </div>

  <script>
    const ctx = document.getElementById('myChart').getContext('2d');
    const myChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'Nhiệt độ (°C)', borderColor: '#38bdf8', backgroundColor: 'rgba(56, 189, 248, 0.1)', data: [], tension: 0.4, fill: true },
          { label: 'Độ ẩm (%)', borderColor: '#10b981', backgroundColor: 'rgba(16, 185, 129, 0.1)', data: [], tension: 0.4, fill: true }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { labels: { color: '#f8fafc', font: { family: 'Outfit' } } }
        },
        scales: {
          x: { ticks: { color: '#94a3b8' }, grid: { color: '#334155' } },
          y: { min: 0, max: 100, ticks: { color: '#94a3b8' }, grid: { color: '#334155' } }
        }
      }
    });

    setInterval(() => {
      fetch('/sensors')
        .then(res => res.json())
        .then(d => {
          const t = parseFloat(d.temp);
          const h = parseFloat(d.hum);
          document.getElementById('temp').innerText = t.toFixed(1);
          document.getElementById('hum').innerText = h.toFixed(1);
          if (d.prediction) {
            document.getElementById('prediction').innerText = d.prediction;
          }
          
          const now = new Date();
          const timeStr = now.getHours() + ':' + String(now.getMinutes()).padStart(2, '0') + ':' + String(now.getSeconds()).padStart(2, '0');
          
          myChart.data.labels.push(timeStr);
          myChart.data.datasets[0].data.push(t);
          myChart.data.datasets[1].data.push(h);
          
          if(myChart.data.labels.length > 20) {
            myChart.data.labels.shift();
            myChart.data.datasets[0].data.shift();
            myChart.data.datasets[1].data.shift();
          }
          myChart.update();
        });
    }, 3000);
  </script>
</body>
</html>)rawliteral";
}

String settingsPage()
{
  return R"rawliteral(<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cài đặt Wi-Fi</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    :root { --bg: #0f172a; --surface: #1e293b; --text: #f8fafc; --primary: #38bdf8; }
    * { box-sizing: border-box; font-family: 'Outfit', sans-serif; }
    body { margin: 0; background-color: var(--bg); color: var(--text); display: flex; align-items: center; justify-content: center; min-height: 100vh; padding: 20px;}
    .container { background: var(--surface); padding: 40px; border-radius: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.6); width: 100%; max-width: 450px; border: 1px solid #334155; }
    h1 { margin-top: 0; margin-bottom: 25px; font-size: 26px; text-align: center; background: linear-gradient(to right, #38bdf8, #818cf8); -webkit-background-clip: text; -webkit-text-fill-color: transparent; font-weight: 700;}
    .form-group { margin-bottom: 20px; }
    label { display: block; margin-bottom: 8px; color: #94a3b8; font-weight: 600; }
    .scan-flex { display: flex; gap: 10px; }
    select, input[type="text"], input[type="password"] { width: 100%; padding: 14px; border-radius: 10px; border: 1px solid #334155; background: var(--bg); color: var(--text); font-size: 16px; outline: none; transition: border-color 0.3s;}
    select:focus, input:focus { border-color: var(--primary); }
    button { padding: 14px 20px; border: none; border-radius: 10px; font-weight: 600; font-size: 16px; cursor: pointer; transition: 0.3s; }
    .btn-scan { background: #334155; color: white; white-space: nowrap; }
    .btn-scan:hover { background: #475569; }
    .btn-submit { background: linear-gradient(135deg, #38bdf8, #818cf8); color: white; width: 100%; margin-top: 10px; box-shadow: 0 4px 15px rgba(56,189,248,0.3);}
    .btn-submit:hover { opacity: 0.9; transform: translateY(-2px); box-shadow: 0 6px 20px rgba(56,189,248,0.4); }
    .btn-back { background: transparent; color: #94a3b8; width: 100%; margin-top: 15px; border: 1px solid #334155; }
    .btn-back:hover { background: #334155; color: white;}
    #msg { margin-top: 15px; text-align: center; font-weight: 600; color: #10b981;}
    .loader { display: none; width: 20px; height: 20px; border: 3px solid rgba(255,255,255,0.3); border-radius: 50%; border-top-color: #fff; animation: spin 1s ease-in-out infinite; margin: 0 auto; }
    @keyframes spin { to { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div class="container">
    <h1>⚙️ Cấu hình Wi-Fi ESP32</h1>
    <form id="wifiForm">
      <div class="form-group">
        <label>Chọn mạng Wi-Fi</label>
        <div class="scan-flex">
          <select id="ssid" onchange="document.getElementById('manualSsid').value = this.value;">
            <option value="">-- Chọn mạng --</option>
          </select>
          <button type="button" class="btn-scan" onclick="scanWifi()" id="btnScan">🔍 Dò mạng</button>
        </div>
      </div>
      <div class="form-group">
        <label>Hoặc nhập SSID (mạng ẩn)</label>
        <input type="text" id="manualSsid" placeholder="Tên Wi-Fi">
      </div>
      <div class="form-group">
        <label>Mật khẩu</label>
        <input type="password" id="pass" placeholder="Bỏ trống nếu không có mật khẩu">
      </div>
      <button type="submit" class="btn-submit">🚀 Lưu và Kết nối</button>
      <button type="button" class="btn-back" onclick="window.location='/'">🏠 Về trang chủ</button>
    </form>
    <div id="msg"><div class="loader" id="loader"></div><div id="msgTxt" style="margin-top:10px;"></div></div>
  </div>
  <script>
    function scanWifi() {
      const btn = document.getElementById('btnScan');
      const select = document.getElementById('ssid');
      btn.innerText = 'Đang dò...'; btn.disabled = true;
      select.innerHTML = '<option value="">Đang tìm mạng xung quanh...</option>';
      fetch('/scan')
        .then(r => r.json())
        .then(data => {
          btn.innerText = '🔍 Làm mới'; btn.disabled = false;
          select.innerHTML = '<option value="">-- Chọn mạng --</option>';
          data.forEach(n => {
            select.innerHTML += `<option value="${n.ssid}">${n.ssid} (${n.rssi} dBm)</option>`;
          });
        })
        .catch(e => {
          btn.innerText = '🔍 Thử lại'; btn.disabled = false;
          select.innerHTML = '<option value="">Lỗi khi dò mạng!</option>';
        });
    }

    document.getElementById('wifiForm').onsubmit = function(e){
      e.preventDefault();
      let ssid = document.getElementById('manualSsid').value.trim();
      if(!ssid) { alert("Vui lòng chọn hoặc nhập tên Wi-Fi!"); return; }
      
      let pass = document.getElementById('pass').value;
      document.getElementById('loader').style.display = 'block';
      document.getElementById('msgTxt').innerText = 'Đang lưu cấu hình và kết nối...';
      
      fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))
        .then(r=>r.text())
        .then(msg=>{
          document.getElementById('loader').style.display = 'none';
          document.getElementById('msgTxt').innerText = msg;
        });
    };
  </script>
</body>
</html>)rawliteral";
}

// ========== Handlers ==========
void handleRoot() { server.send(200, "text/html", mainPage()); }

void handleSensors()
{
  float t = 0.0;
  float h = 0.0;
  String weatherLabel = "Calculating..."; // Biến cục bộ

  SensorData_t receivedData;
  WeatherData_t receivedWeather;

  // Lấy dữ liệu nhiệt ẩm
  if (sensorQueue != NULL && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
      t = receivedData.temperature;
      h = receivedData.humidity;
  }
  
  // Lấy dữ liệu dự đoán thời tiết
  if (weatherQueue != NULL && xQueuePeek(weatherQueue, &receivedWeather, 0) == pdPASS) {
      weatherLabel = String(receivedWeather.label);
  }

  // Cập nhật chuỗi JSON
  String json = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + ",\"prediction\":\"" + weatherLabel + "\"}";
  server.send(200, "application/json", json);
}

void handleWifiScan()
{
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; ++i) {
    if(i) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSettings() { server.send(200, "text/html", settingsPage()); }

void handleConnect()
{
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  server.send(200, "text/plain", "Connecting....");
  isAPMode = false;
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

// ========== WiFi ==========
void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/scan", HTTP_GET, handleWifiScan);
  server.on("/connect", HTTP_GET, handleConnect);
  server.begin();
}

void startAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
}

void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  if (wifi_password.isEmpty())
  {
    WiFi.begin(wifi_ssid.c_str());
  }
  else
  {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  }
  Serial.print("Connecting to: ");
  Serial.print(wifi_ssid.c_str());

  Serial.print(" Password: ");
  Serial.print(wifi_password.c_str());
}

// ========== Main task ==========
void main_server_task(void *pvParameters)
{
  pinMode(BOOT_PIN, INPUT_PULLUP);

  startAP();
  setupServer();

  while (1)
  {
    server.handleClient();

    // BOOT Button to switch to AP Mode
    if (digitalRead(BOOT_PIN) == LOW)
    {
      vTaskDelay(100);
      if (digitalRead(BOOT_PIN) == LOW)
      {
        if (!isAPMode)
        {
          startAP();
          setupServer();
        }
      }
    }

    // STA Mode
    if (connecting)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        isWifiConnected = true; // Internet access

        xSemaphoreGive(xBinarySemaphoreInternet);

        isAPMode = false;
        connecting = false;
      }
      else if (millis() - connect_start_ms > 10000)
      { // timeout 10s
        Serial.println("WiFi connect failed! Back to AP.");
        startAP();
        setupServer();
        connecting = false;
        isWifiConnected = false;
      }
    }

    vTaskDelay(20); // avoid watchdog reset
  }
}