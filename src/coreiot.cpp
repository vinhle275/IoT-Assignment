#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
// Cấu hình CoreIOT
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "xyphyk4n7e9f3g5cg2uh";   // Device Access Token

// Cấu hình TinyMQTT (Local Broker)
const char* tinyMQTT_Server = "192.168.1.19"; // THAY ĐỔI THÀNH IP THỰC TẾ CỦA MÁY CHẠY TINYMQTT
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  int mqttRetryCount = 0;

  // Vòng lặp cho đến khi kết nối được MQTT
  while (!client.connected()) {
    
    // 1. KIỂM TRA ĐIỀU KIỆN MẠNG NGẶT NGHÈO HƠN
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
      Serial.println("Mất mạng hoặc chưa có IP! Đang ép WiFi kết nối lại...");
      WiFi.reconnect(); // Ép hệ thống mạng ESP32 khởi động lại quá trình xin IP
      delay(3000);
      continue; // Quay lại đầu vòng lặp chờ mạng lên
    }

    // 2. ƯU TIÊN KẾT NỐI VỚI TINYMQTT TRƯỚC
    Serial.print("Attempting TinyMQTT connection at ");
    Serial.print(tinyMQTT_Server);
    Serial.println(" ...");
    
    // Chuyển hướng PubSubClient sang TinyMQTT
    client.setServer(tinyMQTT_Server, mqttPort);
    
    // Thử kết nối. Nếu script TinyMQTT.py của bạn yêu cầu auth, hãy thêm user/pass vào hàm connect
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to TinyMQTT!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+ (TinyMQTT)");
      mqttRetryCount = 0; 
      break; // Kết nối thành công, thoát vòng lặp
    } 
    
    Serial.println("TinyMQTT failed. Fallback to CoreIOT Server...");

    // 3. NẾU TINYMQTT LỖI, CHUYỂN SANG COREIOT
    client.setServer(coreIOT_Server, mqttPort);
    Serial.print("Attempting CoreIOT connection...");
    
    if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
      Serial.println("Connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+ (CoreIOT)");
      mqttRetryCount = 0; // Reset bộ đếm lỗi khi thành công
      break; // Kết nối thành công, thoát vòng lặp
      
    } else {
      Serial.print("CoreIOT failed, rc=");
      Serial.print(client.state());
      Serial.println(" - try again in 5 seconds");
      delay(5000);
      mqttRetryCount++;

      // 4. CƠ CHẾ CHỐNG KẸT DNS (Anti-DNS-Lock)
      if (mqttRetryCount >= 5) {
        Serial.println("Kẹt mạng cục bộ! Đang Reset toàn bộ module WiFi...");
        WiFi.disconnect();
        delay(1000);
        WiFi.reconnect();
        mqttRetryCount = 0;
        delay(3000); // Chờ WiFi khởi động lại
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  // Allocate a temporary buffer for the message
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    bool params = doc["params"];

    if (params == true) {
      Serial.println("Device turned ON.");
      if (uxSemaphoreGetCount(xSemaphoreLedControl) == 0) {
          xSemaphoreGive(xSemaphoreLedControl);
      }
    } else {   
      Serial.println("Device turned OFF.");
      xSemaphoreTake(xSemaphoreLedControl, 0);
    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}

void setup_coreiot() {
  while(1) {
    if (xSemaphoreTake(xBinarySemaphoreInternet, /*portMAX_DELAY*/ 0)) {
      xSemaphoreGive(xBinarySemaphoreInternet);
      break;
    }
    delay(500);
    Serial.print("help_me");
  }

  Serial.println(" Connected!");

  // Cấu hình callback nhận message, việc setServer() đã được chuyển vào trong hàm reconnect()
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters) {
  SensorData_t receivedData;

  setup_coreiot();

  while(1) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
        String payload = "{\"temperature\":" + String(receivedData.temperature) +  ",\"humidity\":" + String(receivedData.humidity) + "}";
        
        client.publish("v1/devices/me/telemetry", payload.c_str());
        Serial.println("Published payload: " + payload);
        
    } else {
        // Có thể comment dòng in này lại nếu nó trôi màn hình console quá nhiều
        // Serial.println("Sensor queue is empty or unavailable!");
    }

    vTaskDelay(10000);  // Publish every 10 seconds
  }
}