#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
// Cấu hình CoreIOT
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "2a9lqf9pvmor6aui0bts";   // Device Access Token

// Cấu hình định danh thiết bị và bảo mật Local
const char* DEVICE_ID = "ESP32_001";
// Khớp với SHARED_TOKEN của Gateway
const char* MQTT_PASS = "1234567890"; 

// Cấu hình TinyMQTT (Local Broker)
const char* tinyMQTT_Server = "192.168.1.3"; // THAY ĐỔI THÀNH IP THỰC TẾ CỦA MÁY CHẠY TINYMQTT
const int   mqttPort = 1883;

// Cấu hình Tọa độ cố định (Ví dụ: Trung tâm TP.HCM)
const float FIXED_LAT = 10.772175109674038;
const float FIXED_LON = 106.65789107082472;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

// Cờ nhận diện đường truyền (Nối qua Gateway hay nối thẳng Cloud)
bool isDirectCoreIOT = false; 

void reconnect() {
  int mqttRetryCount = 0;

  // Vòng lặp cho đến khi kết nối được MQTT
  while (!client.connected()) {
    
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
      Serial.println("Mất mạng hoặc chưa có IP! Đang ép WiFi kết nối lại...");
      WiFi.reconnect(); 
      delay(3000);
      continue; 
    }

    // 1. ƯU TIÊN KẾT NỐI VỚI TINYMQTT TRƯỚC
    Serial.print("Attempting TinyMQTT connection at ");
    Serial.print(tinyMQTT_Server);
    Serial.println(" ...");
    
    client.setServer(tinyMQTT_Server, mqttPort);
    
    // Sử dụng DEVICE_ID làm Username và MQTT_PASS làm Password
    if (client.connect(DEVICE_ID)) {
      Serial.println("Connected to TinyMQTT!");
      isDirectCoreIOT = false; // Xác nhận đang đi qua Gateway
      
      String rpcTopic = "v1/devices/" + String(DEVICE_ID) + "/rpc/request/+";
      client.subscribe(rpcTopic.c_str());
      
      Serial.println("Subscribed to " + rpcTopic);
      mqttRetryCount = 0; 
      break; 
    } 
    
    Serial.println("TinyMQTT failed. Fallback to CoreIOT Server...");

    // 2. NẾU TINYMQTT LỖI, CHUYỂN SANG COREIOT
    client.setServer(coreIOT_Server, mqttPort);
    Serial.print("Attempting CoreIOT connection...");
    
    if (client.connect(DEVICE_ID, coreIOT_Token, NULL)) {
      Serial.println("Connected to CoreIOT Server!");
      isDirectCoreIOT = true; // Xác nhận đang nối thẳng lên Cloud
      
      // Dùng "me" để CoreIOT Server chấp nhận
      String rpcTopic = "v1/devices/me/rpc/request/+"; 
      client.subscribe(rpcTopic.c_str());
      
      Serial.println("Subscribed to " + rpcTopic);
      mqttRetryCount = 0; 
      break; 
      
    } else {
      Serial.print("CoreIOT failed, rc=");
      Serial.print(client.state());
      Serial.println(" - try again in 5 seconds");
      delay(5000);
      mqttRetryCount++;

      if (mqttRetryCount >= 5) {
        Serial.println("Kẹt mạng cục bộ! Đang Reset toàn bộ module WiFi...");
        WiFi.disconnect();
        delay(1000);
        WiFi.reconnect();
        mqttRetryCount = 0;
        delay(3000); 
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

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
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY )) {
      xSemaphoreGive(xBinarySemaphoreInternet);
      break;
    }
    delay(500);
    Serial.print("waiting_wifi...");
  }
  Serial.println(" Connected!");
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters) {
  SensorData_t receivedData;
  WeatherData_t receivedWeather; // Khai báo biến nhận weather
  
  setup_coreiot();

  while(1) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Peek xem có dữ liệu nhiệt ẩm không
    if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
        
        // Peek xem có dữ liệu ML không
        String weatherStr = "Calculating...";
        if (xQueuePeek(weatherQueue, &receivedWeather, 0) == pdPASS) {
            weatherStr = String(receivedWeather.label);
        }

        // Thêm trường "weather" vào chuỗi JSON payload
        String payload = "{\"temperature\":" + String(receivedData.temperature) + 
                         ",\"humidity\":" + String(receivedData.humidity) + 
                         ",\"weather\":\"" + weatherStr + "\"" +          // Thêm dữ liệu nhãn thời tiết
                         ",\"latitude\":" + String(FIXED_LAT, 6) + 
                         ",\"longitude\":" + String(FIXED_LON, 6) + 
                         ",\"token\":\"" + String(MQTT_PASS) + "\"}";
        
        String telemetryTopic;
        
        // Cấp đúng định dạng Topic theo đường kết nối
        if (isDirectCoreIOT) {
            telemetryTopic = "v1/devices/me/telemetry"; 
        } else {
            telemetryTopic = "v1/devices/" + String(DEVICE_ID) + "/telemetry"; 
        }
        
        client.publish(telemetryTopic.c_str(), payload.c_str());
        Serial.println("Published to " + telemetryTopic + ": " + payload);
        
    }
    vTaskDelay(10000);  
  }
}