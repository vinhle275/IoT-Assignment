#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>

bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;

WebServer server(80);

unsigned long connect_start_ms = 0;
bool connecting = false;


String mainPage()
{
  float temperature = 0.0;
  float humidity = 0.0;
  SensorData_t receivedData;

  if (sensorQueue != NULL && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
      temperature = receivedData.temperature;
      humidity = receivedData.humidity;
  }

  return R"rawliteral(
  <!DOCTYPE html>
  <html lang="vi">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js"></script>
    <style>
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        padding: 20px;
      }
      .header {
        text-align: center;
        color: white;
        margin-bottom: 30px;
      }
      .header h1 {
        font-size: 2.5em;
        font-weight: 700;
        margin-bottom: 10px;
      }
      .container { max-width: 1400px; margin: 0 auto; }
      .grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
        gap: 20px;
        margin-bottom: 30px;
      }
      .card {
        background: white;
        border-radius: 15px;
        padding: 25px;
        box-shadow: 0 10px 30px rgba(0,0,0,0.2);
      }
      .card-title {
        font-size: 1.3em;
        font-weight: 700;
        margin-bottom: 20px;
        color: #333;
      }
      .sensor-big {
        text-align: center;
        padding: 20px;
      }
      .sensor-value {
        font-size: 3.5em;
        font-weight: 800;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        background-clip: text;
        margin: 10px 0;
      }
      .sensor-unit {
        font-size: 1.2em;
        color: #999;
      }
      .sensor-label {
        font-size: 1em;
        color: #666;
        margin-bottom: 15px;
      }
      .status-box {
        background: #f5f5f5;
        border-radius: 10px;
        padding: 15px;
        margin-top: 15px;
        text-align: center;
      }
      .status-text {
        font-weight: 600;
        color: #667eea;
        font-size: 1.1em;
      }
      button {
        padding: 12px 20px;
        border: none;
        border-radius: 10px;
        font-size: 1em;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.3s;
        width: 100%;
        margin-top: 15px;
      }
      .btn-primary {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
      }
      .btn-primary:hover { transform: scale(1.02); }
      .btn-danger {
        background: #ff6b6b;
        color: white;
      }
      .btn-danger:hover { background: #ff5252; }
      .chart-wrapper {
        position: relative;
        height: 350px;
        width: 100%;
        margin-top: 20px;
      }
      .charts-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(480px, 1fr));
        gap: 20px;
      }
      @media (max-width: 768px) {
        .grid { grid-template-columns: 1fr; }
        .charts-grid { grid-template-columns: 1fr; }
        .header h1 { font-size: 1.8em; }
        .sensor-value { font-size: 2.5em; }
      }
    </style>
  </head>
  <body>
    <div class="header">
      <h1>📡 IoT Monitoring System</h1>
      <p style="opacity: 0.9;">Real-time Environment Monitoring</p>
    </div>

    <div class="container">
      <div class="grid">
        <div class="card">
          <div class="card-title">🌡️ Temperature</div>
          <div class="sensor-big">
            <div class="sensor-label">Current Reading</div>
            <div class="sensor-value"><span id="temp">)rawliteral" + String(temperature, 2) + R"rawliteral(</span></div>
            <div class="sensor-unit">°C</div>
            <div class="status-box">
              <div class="status-text" id="tempStatus">Normal</div>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">💧 Humidity</div>
          <div class="sensor-big">
            <div class="sensor-label">Current Reading</div>
            <div class="sensor-value"><span id="hum">)rawliteral" + String(humidity, 2) + R"rawliteral(</span></div>
            <div class="sensor-unit">%</div>
            <div class="status-box">
              <div class="status-text" id="humStatus">Normal</div>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">⚡ System Info</div>
          <div style="padding: 15px;">
            <div style="margin-bottom: 15px;">
              <div style="color: #999; font-size: 0.9em;">Device Mode</div>
              <div style="font-size: 1.3em; font-weight: 700; color: #667eea;">Connected</div>
            </div>
            <div style="margin-bottom: 15px;">
              <div style="color: #999; font-size: 0.9em;">Status</div>
              <div style="font-size: 1.3em; font-weight: 700; color: #4CAF50;">Active</div>
            </div>
            <button class="btn-primary" onclick="window.location='/settings'">⚙️ WiFi Settings</button>
            <button class="btn-danger" onclick="if(confirm('Reset to AP mode?')) fetch('/resetAP').then(()=>setTimeout(()=>window.location='/',1500))">🔄 Reset to AP</button>
          </div>
        </div>
      </div>

      <div class="charts-grid">
        <div class="card">
          <div class="card-title">📈 Temperature Trend</div>
          <div class="chart-wrapper">
            <canvas id="tempChart"></canvas>
          </div>
        </div>
        <div class="card">
          <div class="card-title">📊 Humidity Trend</div>
          <div class="chart-wrapper">
            <canvas id="humChart"></canvas>
          </div>
        </div>
      </div>
    </div>

    <script>
      let tempChart, humChart;
      const maxDataPoints = 30;
      let chartData = {
        labels: [],
        temps: [],
        hums: [],
        timestamps: []
      };

      function initCharts() {
        const tempCtx = document.getElementById('tempChart').getContext('2d');
        const humCtx = document.getElementById('humChart').getContext('2d');

        tempChart = new Chart(tempCtx, {
          type: 'line',
          data: {
            labels: chartData.labels,
            datasets: [{
              label: 'Temperature (°C)',
              data: chartData.temps,
              borderColor: '#ff6b6b',
              backgroundColor: 'rgba(255, 107, 107, 0.08)',
              borderWidth: 3,
              fill: true,
              tension: 0.4,
              pointRadius: 5,
              pointHoverRadius: 7,
              pointBackgroundColor: '#ff6b6b'
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: { display: true, labels: { font: { size: 13, weight: 'bold' }, padding: 15 } }
            },
            scales: {
              y: { min: 15, max: 40, grid: { color: 'rgba(0,0,0,0.05)' } },
              x: { grid: { display: false } }
            }
          }
        });

        humChart = new Chart(humCtx, {
          type: 'line',
          data: {
            labels: chartData.labels,
            datasets: [{
              label: 'Humidity (%)',
              data: chartData.hums,
              borderColor: '#4ecdc4',
              backgroundColor: 'rgba(78, 205, 196, 0.08)',
              borderWidth: 3,
              fill: true,
              tension: 0.4,
              pointRadius: 5,
              pointHoverRadius: 7,
              pointBackgroundColor: '#4ecdc4'
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              legend: { display: true, labels: { font: { size: 13, weight: 'bold' }, padding: 15 } }
            },
            scales: {
              y: { min: 0, max: 100, grid: { color: 'rgba(0,0,0,0.05)' } },
              x: { grid: { display: false } }
            }
          }
        });
      }

      function getStatus(temp, hum) {
        let tempStatus = 'Normal';
        if (temp > 35) tempStatus = '🔥 Hot';
        else if (temp > 30) tempStatus = '⚠️ Warm';
        else if (temp < 15) tempStatus = '❄️ Cold';
        
        let humStatus = 'Normal';
        if (hum > 75) humStatus = '💦 Very Humid';
        else if (hum > 60) humStatus = '💧 Humid';
        else if (hum < 25) humStatus = '🏜️ Very Dry';

        return { tempStatus, humStatus };
      }

      function updateSensors() {
        fetch('/sensors')
          .then(res => res.json())
          .then(data => {
            const temp = parseFloat(data.temp);
            const hum = parseFloat(data.hum);
            
            document.getElementById('temp').innerText = temp.toFixed(1);
            document.getElementById('hum').innerText = hum.toFixed(1);
            
            const { tempStatus, humStatus } = getStatus(temp, hum);
            document.getElementById('tempStatus').innerText = tempStatus;
            document.getElementById('humStatus').innerText = humStatus;

            const now = new Date();
            const timeStr = now.getHours().toString().padStart(2,'0') + ':' + 
                           now.getMinutes().toString().padStart(2,'0') + ':' +
                           now.getSeconds().toString().padStart(2,'0');

            chartData.labels.push(timeStr);
            chartData.temps.push(temp);
            chartData.hums.push(hum);

            if (chartData.labels.length > maxDataPoints) {
              chartData.labels.shift();
              chartData.temps.shift();
              chartData.hums.shift();
            }

            tempChart.data.labels = chartData.labels;
            tempChart.data.datasets[0].data = chartData.temps;
            tempChart.update('none');

            humChart.data.labels = chartData.labels;
            humChart.data.datasets[0].data = chartData.hums;
            humChart.update('none');
          })
          .catch(e => console.error('Fetch error:', e));
      }

      initCharts();
      updateSensors();
      setInterval(updateSensors, 1000);
    </script>
  </body>
  </html>
  )rawliteral";
}

String settingsPage()
{
  return R"rawliteral(
  <!DOCTYPE html>
  <html lang="vi">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Settings</title>
    <style>
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 20px;
      }
      .container {
        background: white;
        border-radius: 15px;
        padding: 40px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        width: 100%;
        max-width: 450px;
      }
      h1 {
        color: #333;
        margin-bottom: 10px;
        text-align: center;
      }
      .subtitle {
        text-align: center;
        color: #999;
        margin-bottom: 30px;
        font-size: 0.95em;
      }
      .form-group {
        margin-bottom: 20px;
      }
      label {
        display: block;
        margin-bottom: 8px;
        color: #333;
        font-weight: 600;
      }
      input[type="text"], input[type="password"], select {
        width: 100%;
        padding: 12px;
        border: 2px solid #eee;
        border-radius: 8px;
        font-size: 1em;
        transition: border-color 0.3s;
        font-family: inherit;
      }
      input:focus, select:focus {
        outline: none;
        border-color: #667eea;
      }
      .wifi-list {
        background: #f5f5f5;
        border: 2px solid #eee;
        border-radius: 8px;
        max-height: 250px;
        overflow-y: auto;
        margin-bottom: 15px;
      }
      .wifi-item {
        padding: 12px;
        border-bottom: 1px solid #eee;
        cursor: pointer;
        transition: background 0.2s;
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
      .wifi-item:last-child { border-bottom: none; }
      .wifi-item:hover { background: #e8e8e8; }
      .wifi-name { font-weight: 600; color: #333; }
      .wifi-strength {
        font-size: 0.85em;
        color: #666;
        margin-top: 3px;
      }
      .signal-bars { color: #667eea; font-size: 0.9em; }
      .scan-btn {
        width: 100%;
        padding: 10px;
        background: #667eea;
        color: white;
        border: none;
        border-radius: 8px;
        font-weight: 600;
        cursor: pointer;
        margin-bottom: 15px;
        transition: all 0.3s;
      }
      .scan-btn:hover { background: #764ba2; }
      .scan-btn:disabled {
        background: #ccc;
        cursor: not-allowed;
      }
      .button-group {
        display: flex;
        gap: 10px;
      }
      .button-group button {
        flex: 1;
        padding: 12px;
        border: none;
        border-radius: 8px;
        font-size: 1em;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.3s;
      }
      .btn-connect {
        background: #4CAF50;
        color: white;
      }
      .btn-connect:hover { background: #45a049; }
      .btn-back {
        background: #f0f0f0;
        color: #667eea;
      }
      .btn-back:hover { background: #e0e0e0; }
      .loading {
        text-align: center;
        color: #667eea;
        font-weight: 600;
        margin: 15px 0;
      }
      .spinner {
        display: inline-block;
        width: 20px;
        height: 20px;
        border: 3px solid #f3f3f3;
        border-top: 3px solid #667eea;
        border-radius: 50%;
        animation: spin 1s linear infinite;
        margin-right: 10px;
      }
      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }
      .message {
        padding: 12px;
        border-radius: 8px;
        margin-top: 20px;
        text-align: center;
        font-weight: 600;
        display: none;
      }
      .message.success {
        background: #d4edda;
        color: #155724;
        display: block;
      }
      .message.error {
        background: #f8d7da;
        color: #721c24;
        display: block;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>📡 WiFi Settings</h1>
      <p class="subtitle">Connect to your network</p>

      <button class="scan-btn" id="scanBtn" onclick="scanWiFi()">🔍 Scan WiFi Networks</button>

      <div id="loadingDiv" class="loading" style="display:none;">
        <span class="spinner"></span>Scanning...
      </div>

      <div id="wifiListDiv" style="display:none;">
        <label>Available Networks:</label>
        <div class="wifi-list" id="wifiList"></div>
      </div>

      <form id="wifiForm" onsubmit="connectWiFi(event)">
        <div class="form-group">
          <label for="ssid">Network Name (SSID)</label>
          <input type="text" id="ssid" name="ssid" placeholder="Enter or select from list" required>
        </div>
        <div class="form-group">
          <label for="pass">Password</label>
          <input type="password" id="pass" name="password" placeholder="Leave blank if no password">
        </div>
        
        <div class="button-group">
          <button type="submit" class="btn-connect">✓ Connect</button>
          <button type="button" class="btn-back" onclick="window.location='/'">← Back</button>
        </div>
      </form>

      <div id="message" class="message"></div>
    </div>

    <script>
      function scanWiFi() {
        const scanBtn = document.getElementById('scanBtn');
        const loadingDiv = document.getElementById('loadingDiv');
        const wifiListDiv = document.getElementById('wifiListDiv');
        
        scanBtn.disabled = true;
        loadingDiv.style.display = 'block';
        wifiListDiv.style.display = 'none';

        fetch('/scan')
          .then(res => res.json())
          .then(networks => {
            loadingDiv.style.display = 'none';
            
            if (networks.length === 0) {
              document.getElementById('message').textContent = 'No WiFi networks found';
              document.getElementById('message').className = 'message error';
              return;
            }

            const wifiList = document.getElementById('wifiList');
            wifiList.innerHTML = '';
            
            networks.forEach(net => {
              const signals = ['📶', '📶📶', '📶📶📶'];
              const signalIndex = Math.min(2, Math.floor((net.rssi + 100) / 25));
              const div = document.createElement('div');
              div.className = 'wifi-item';
              div.innerHTML = `
                <div>
                  <div class="wifi-name">${net.ssid || '(Hidden)'}</div>
                  <div class="wifi-strength"><span class="signal-bars">${signals[signalIndex]}</span> ${net.rssi} dBm</div>
                </div>
                <span style="cursor: pointer; color: #667eea;">→</span>
              `;
              div.onclick = () => {
                document.getElementById('ssid').value = net.ssid;
                document.getElementById('pass').focus();
              };
              wifiList.appendChild(div);
            });
            
            wifiListDiv.style.display = 'block';
            scanBtn.disabled = false;
          })
          .catch(e => {
            loadingDiv.style.display = 'none';
            scanBtn.disabled = false;
            document.getElementById('message').textContent = 'Scan failed: ' + e;
            document.getElementById('message').className = 'message error';
          });
      }

      function connectWiFi(event) {
        event.preventDefault();
        const ssid = document.getElementById('ssid').value;
        const pass = document.getElementById('pass').value;
        
        document.getElementById('message').textContent = 'Connecting...';
        document.getElementById('message').className = 'message';
        
        fetch('/connect?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
          .then(res => res.text())
          .then(msg => {
            document.getElementById('message').textContent = 'Connected! Redirecting...';
            document.getElementById('message').className = 'message success';
            setTimeout(() => window.location = '/', 2000);
          })
          .catch(e => {
            document.getElementById('message').textContent = 'Connection failed: ' + e;
            document.getElementById('message').className = 'message error';
          });
      }
    </script>
  </body>
  </html>
  )rawliteral";
}

// ========== Handlers ==========
void handleRoot() { server.send(200, "text/html", mainPage()); }

void handleToggle()
{
  int led = server.arg("led").toInt();
  if (led == 1)
  {
    led1_state = !led1_state;
    Serial.println("YOUR CODE TO CONTROL LED1");
  }
  else if (led == 2)
  {
    led2_state = !led2_state;
    Serial.println("YOUR CODE TO CONTROL LED2");
  }
  server.send(200, "application/json",
              "{\"led1\":\"" + String(led1_state ? "ON" : "OFF") +
                  "\",\"led2\":\"" + String(led2_state ? "ON" : "OFF") + "\"}");
}

void handleSensors()
{
  float t = 0.0;
  float h = 0.0;
  SensorData_t receivedData;

  if (sensorQueue != NULL && xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
      t = receivedData.temperature;
      h = receivedData.humidity;
  }
  String json = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + "}";
  server.send(200, "application/json", json);
}

void handleScan()
{
  int n = WiFi.scanNetworks();
  String json = "[";
  
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
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

void handleResetAP()
{
  server.send(200, "text/plain", "Resetting to AP mode...");
  isAPMode = false; // Force reset
  vTaskDelay(500);
  startAP();
  setupServer();
  isWifiConnected = false;
  connecting = false;
}

// ========== WiFi ==========
void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/resetAP", HTTP_GET, handleResetAP);
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
