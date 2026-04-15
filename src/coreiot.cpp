#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "xyphyk4n7e9f3g5cg2uh";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);
void reconnect() {
  int mqttRetryCount = 0;

  // Vòng lặp cho đến khi kết nối được MQTT
  while (!client.connected()) {
    
    // 1. KIỂM TRA ĐIỀU KIỆN MẠNG NGẶT NGHÈO HƠN (Có sóng VÀ phải có IP thực)
    if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
      Serial.println("Mất mạng hoặc chưa có IP! Đang ép WiFi kết nối lại...");
      WiFi.reconnect(); // Ép hệ thống mạng ESP32 khởi động lại quá trình xin IP
      delay(3000);
      continue; // Quay lại đầu vòng lặp chờ mạng lên
    }

    Serial.print("Attempting MQTT connection...");
    
    // 2. THỰC HIỆN KẾT NỐI MQTT
    if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
      Serial.println("connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+");
      mqttRetryCount = 0; // Reset bộ đếm lỗi khi thành công

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      mqttRetryCount++;

      // 3. CƠ CHẾ CHỐNG KẸT DNS (Anti-DNS-Lock)
      // Nếu cố kết nối MQTT thất bại 5 lần liên tục (thường là do kẹt DNS sâu bên trong LwIP)
      // Chúng ta sẽ "rút phích cắm" WiFi và cắm lại.
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
// void reconnect() {
//   // Loop until we're reconnected
//   while (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     // Attempt to connect (username=token, password=empty)
//     if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
//       Serial.println("connected to CoreIOT Server!");
//       client.subscribe("v1/devices/me/rpc/request/+");
//       Serial.println("Subscribed to v1/devices/me/rpc/request/+");

//     } else {
//       Serial.print("failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" try again in 5 seconds");
//       delay(5000);
//     }
//   }
// }


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
    // Check params type (could be boolean, int, or string according to your RPC)
    // Example: {"method": "setValueLED", "params": "ON"}
    const char* params = doc["params"];

    if (strcmp(params, "ON") == 0) {
      Serial.println("Device turned ON.");
      //TODO

    } else {   
      Serial.println("Device turned OFF.");
      //TODO

    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}


void setup_coreiot(){

  //Serial.print("Connecting to WiFi...");
  //WiFi.begin(wifi_ssid, wifi_password);
  //while (WiFi.status() != WL_CONNECTED) {
  
  // while (isWifiConnected == false) {
  //   delay(500);
  //   Serial.print(".");
  // }

  while(1){
    if (xSemaphoreTake(xBinarySemaphoreInternet, /*portMAX_DELAY*/ 0)) {
      break;
    }
    delay(500);
    Serial.print("help_me");
  }


  Serial.println(" Connected!");

  client.setServer(coreIOT_Server, mqttPort);
  client.setCallback(callback);

}

void coreiot_task(void *pvParameters){

  SensorData_t receivedData;

    setup_coreiot();

    while(1){

        if (!client.connected()) {
            reconnect();
        }
        client.loop();


        if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
            
            String payload = "{\"temperature\":" + String(receivedData.temperature) +  ",\"humidity\":" + String(receivedData.humidity) + "}";
            
            client.publish("v1/devices/me/telemetry", payload.c_str());
            Serial.println("Published payload: " + payload);
            
        } else {
            Serial.println("Sensor queue is empty or unavailable!");
        }

        vTaskDelay(10000);  // Publish every 10 seconds
    }
}