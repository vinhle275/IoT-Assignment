#include "coreiot.h"

// Biến cố định cho tọa độ (Giữ nguyên)
const float FIXED_LAT = 10.772175109674038;
const float FIXED_LON = 106.65789107082472;

WiFiClient espClient;
PubSubClient client(espClient);

bool isDirectCoreIOT = false;

void reconnect() {
  int mqttRetryCount = 0;
  
  // Khai báo struct để chứa dữ liệu cấu hình từ Queue
  CoreIotConfig_t coreCfg;
  MqttLocalConfig_t localCfg;

  while (!client.connected()) {
    // Lấy cấu hình mới nhất từ Queue bằng xQueuePeek
    if (xQueuePeek(coreIotQueue, &coreCfg, 0) != pdPASS || 
        xQueuePeek(localMqttQueue, &localCfg, 0) != pdPASS) {
      Serial.println("Waiting for configuration from Queues...");
      delay(2000);
      continue;
    }

    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
      Serial.println("Mất kết nối mạng! Đang chờ WiFi...");
      WiFi.reconnect();
      delay(3000);
      continue;
    }

    // 1. ƯU TIÊN KẾT NỐI TINYMQTT (LOCAL) TRƯỚC
    Serial.print("Attempting TinyMQTT connection at ");
    Serial.print(localCfg.tiny_server);
    Serial.println(" ...");
    
    client.setServer(localCfg.tiny_server, localCfg.port);

    // Sử dụng device_id làm ClientID
    if (client.connect(localCfg.device_id)) {
      Serial.println("Connected to TinyMQTT!");
      isDirectCoreIOT = false;
      
      String rpcTopic = "v1/devices/" + String(localCfg.device_id) + "/rpc/request/+";
      client.subscribe(rpcTopic.c_str());
      
      Serial.println("Subscribed to " + rpcTopic);
      mqttRetryCount = 0;
      break;
    }

    Serial.println("TinyMQTT failed. Fallback to CoreIOT Server...");

    // 2. NẾU TINYMQTT LỖI, CHUYỂN SANG COREIOT CLOUD
    client.setServer(coreCfg.server, localCfg.port);
    Serial.print("Attempting CoreIOT connection to ");
    Serial.println(coreCfg.server);

    if (client.connect(localCfg.device_id, coreCfg.token, NULL)) {
      Serial.println("Connected to CoreIOT Server!");
      isDirectCoreIOT = true;
      
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
        Serial.println("Kết nối thất bại, đang Reset module WiFi...");
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
  
  if (method && strcmp(method, "setStateLED") == 0) {
    bool params = doc["params"];
    if (params == true) {
      Serial.println("Device LED turned ON.");
      if (uxSemaphoreGetCount(xSemaphoreLedControl) == 0) {
          xSemaphoreGive(xSemaphoreLedControl);
      }
    } else {
      Serial.println("Device LED turned OFF.");
      xSemaphoreTake(xSemaphoreLedControl, 0);
    }
  } 
  // Thêm method điều khiển NeoLED tại đây
  else if (method && strcmp(method, "setStateNEO") == 0) {
    bool params = doc["params"];
    if (params == true) {
      Serial.println("NeoLED turned ON.");
      // Kiểm tra nếu Semaphore chưa được Give thì mới Give
      if (uxSemaphoreGetCount(xSemaphoreNeoControl) == 0) {
          xSemaphoreGive(xSemaphoreNeoControl);
      }
    } else {
      Serial.println("NeoLED turned OFF.");
      // Take Semaphore để ra lệnh tắt
      xSemaphoreTake(xSemaphoreNeoControl, 0);
    }
  } 
  else {
    Serial.print("Unknown method: ");
    Serial.println(method ? method : "NULL");
  }
}

void setup_coreiot() {
  while(1) {
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
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
  WeatherData_t receivedWeather;
  MqttLocalConfig_t localCfg; // Để lấy MQTT_PASS (token) khi gửi telemetry

  setup_coreiot();

  while(1) {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Lấy dữ liệu cảm biến
    if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
        
        // Lấy dữ liệu dự báo thời tiết từ TinyML
        String weatherStr = "Calculating...";
        if (xQueuePeek(weatherQueue, &receivedWeather, 0) == pdPASS) {
            weatherStr = String(receivedWeather.label);
        }

        // Lấy cấu hình Local để lấy mật khẩu/token thiết bị
        String deviceToken = "1234567890"; // Giá trị mặc định
        if (xQueuePeek(localMqttQueue, &localCfg, 0) == pdPASS) {
            deviceToken = String(localCfg.mqtt_pass);
        }

        // Tạo JSON payload
        String payload = "{\"temperature\":" + String(receivedData.temperature) +
                          ",\"humidity\":" + String(receivedData.humidity) +
                          ",\"weather\":\"" + weatherStr + "\"" +
                          ",\"latitude\":" + String(FIXED_LAT, 6) +
                          ",\"longitude\":" + String(FIXED_LON, 6) +
                          ",\"token\":\"" + deviceToken + "\"}";

        String telemetryTopic;
        
        // Xác định Topic dựa trên cấu hình hiện tại
        if (isDirectCoreIOT) {
            telemetryTopic = "v1/devices/me/telemetry";
        } else {
            // Nếu qua Gateway, dùng DeviceID từ queue local
            telemetryTopic = "v1/devices/" + String(localCfg.device_id) + "/telemetry";
        }

        client.publish(telemetryTopic.c_str(), payload.c_str());
        Serial.println("Published to " + telemetryTopic + ": " + payload);
    }
    
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}