#include "neo_blinky.h"


// void neo_blinky(void *pvParameters){

//     Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
//     strip.begin();
//     // Set all pixels to off to start
//     strip.clear();
//     strip.show();

//     while(1) {                          
//         strip.setPixelColor(0, strip.Color(255, 0, 0)); // Set pixel 0 to red
//         strip.show(); // Update the strip

//         // Wait for 500 milliseconds
//         vTaskDelay(500);

//         // Set the pixel to off
//         strip.setPixelColor(0, strip.Color(0, 0, 0)); // Turn pixel 0 off
//         strip.show(); // Update the strip

//         // Wait for another 500 milliseconds
//         vTaskDelay(500);
//     }
// }




// void neo_blinky(void *pvParameters) {

//     Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
//     strip.begin();
//     strip.clear();
//     strip.setBrightness(50); 
//     strip.show();
    
//     SensorData_t receivedData;
//     NeoBlinkyState_t currentState = LED_STATE_NORMAL_HUMI;
    
//     TickType_t blinkInterval = pdMS_TO_TICKS(1000); 
//     TickType_t lastToggleTime = xTaskGetTickCount();
//     bool ledState = false;
//     uint32_t currentColor = strip.Color(0, 255, 0); // Mặc định Lục

//     while (1) {
//         if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
            
//             if (receivedData.humidity >= HUMIDITY_VERY_HUMID) {
//                 currentState = LED_STATE_HUMIDITY_VERY_HUMID;
//             } 
//             else if (receivedData.humidity >= HUMIDITY_HUMID) {
//                 currentState = LED_STATE_HUMIDITY_HUMID;
//             } 
//             else if (receivedData.humidity <= HUMIDITY_VERY_DRY) {
//                 currentState = LED_STATE_HUMIDITY_VERY_DRY;
//             } 
//             else if (receivedData.humidity <= HUMIDITY_DRY) {
//                 currentState = LED_STATE_HUMIDITY_DRY;
//             } 
//             else {
//                 currentState = LED_STATE_NORMAL_HUMI; 
//             }
//         }





//         switch (currentState) {
//             case LED_STATE_HUMIDITY_VERY_HUMID:  
//                 currentColor = strip.Color(128, 0, 128);      
//                 break;
//             case LED_STATE_HUMIDITY_HUMID: 
//                 currentColor = strip.Color(0, 0, 255);         
//                 break;
//             case LED_STATE_NORMAL_HUMI:    
//                 currentColor = strip.Color(0, 255, 0);      
//                 break;
//             case LED_STATE_HUMIDITY_DRY: 
//                 currentColor = strip.Color(255, 255, 0);     
//                 break;
//             case LED_STATE_HUMIDITY_VERY_DRY: 
//                 currentColor = strip.Color(255, 0, 0);    
//                 break;
//         }




//         // Biến static để hạn chế in ra Serial quá nhiều (chỉ in mỗi 1 giây)
//         // static TickType_t lastDebugTime = 0;
//         // if (xTaskGetTickCount() - lastDebugTime > pdMS_TO_TICKS(1000)) {
//         //     if (xBinarySemaphoreInternet == NULL) {
//         //         Serial.println("[LED Task] LỖI: xBinarySemaphoreInternet chưa được khởi tạo (NULL)!");
//         //     } else {
//         //         Serial.printf("[LED Task] Trạng thái Semaphore Count: %d\n", uxSemaphoreGetCount(xBinarySemaphoreInternet));
//         //     }
//         //     lastDebugTime = xTaskGetTickCount();
//         // }

//         // Logic điều khiển LED
//         if (xBinarySemaphoreInternet == NULL || uxSemaphoreGetCount(xBinarySemaphoreInternet) == 0) {
//             // MẤT MẠNG -> Nhấp nháy
//             blinkInterval = pdMS_TO_TICKS(500);
            
//             if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
//                 ledState = !ledState;
//                 if (ledState) {
//                     strip.setPixelColor(0, currentColor); 
//                 } else {
//                     strip.setPixelColor(0, strip.Color(0, 0, 0)); 
//                 }
//                 strip.show();
//                 lastToggleTime = xTaskGetTickCount();
//             }
//         } else {
//             // CÓ MẠNG -> Sáng cố định
//             strip.setPixelColor(0, currentColor);
//             strip.show();
//             ledState = true; 
//         }




//         // if (/*uxSemaphoreGetCount(xBinarySemaphoreInternet) == 0*/ WiFi.status() != WL_CONNECTED) {
//         //     blinkInterval = pdMS_TO_TICKS(500);
            
//         //     if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
//         //         ledState = !ledState;
//         //         if (ledState) {
//         //             strip.setPixelColor(0, currentColor); // Hoặc bạn có thể hardcode strip.Color(255,255,255) ở đây nếu muốn nháy màu trắng
//         //         } else {
//         //             strip.setPixelColor(0, strip.Color(0, 0, 0)); 
//         //         }
//         //         strip.show();
//         //         lastToggleTime = xTaskGetTickCount();
//         //     }
//         // } else {
//         //     // Có Wi-Fi -> Sáng cố định theo màu của độ ẩm, không nhấp nháy
//         //     strip.setPixelColor(0, currentColor);
//         //     strip.show();
//         //     ledState = true; // Cập nhật lại ledState để giữ logic đồng bộ nếu rớt mạng lại
//         // }


//         vTaskDelay(pdMS_TO_TICKS(10)); 
//     }
// }


void neo_blinky(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.setBrightness(50);
    strip.show();
    
    SensorData_t receivedData;
    NeoBlinkyState_t currentState = LED_STATE_NORMAL_HUMI;
    uint32_t currentColor = strip.Color(0, 255, 0);

    while (1) {
        // Kiểm tra Semaphore điều khiển trước khi làm bất cứ việc gì
        if (xSemaphoreTake(xSemaphoreNeoControl, 0) == pdTRUE) {
            // Trả lại semaphore ngay để trạng thái vẫn là "ON" cho các vòng lặp sau
            xSemaphoreGive(xSemaphoreNeoControl);

            // Logic đọc độ ẩm để quyết định màu sắc (Giữ nguyên logic của bạn)
            if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS) {
                if (receivedData.humidity >= HUMIDITY_VERY_HUMID) currentState = LED_STATE_HUMIDITY_VERY_HUMID;
                else if (receivedData.humidity >= HUMIDITY_HUMID) currentState = LED_STATE_HUMIDITY_HUMID;
                else if (receivedData.humidity <= HUMIDITY_VERY_DRY) currentState = LED_STATE_HUMIDITY_VERY_DRY;
                else if (receivedData.humidity <= HUMIDITY_DRY) currentState = LED_STATE_HUMIDITY_DRY;
                else currentState = LED_STATE_NORMAL_HUMI;
            }

            switch (currentState) {
                case LED_STATE_HUMIDITY_VERY_HUMID: currentColor = strip.Color(128, 0, 128); break;
                case LED_STATE_HUMIDITY_HUMID:      currentColor = strip.Color(0, 0, 255); break;
                case LED_STATE_NORMAL_HUMI:        currentColor = strip.Color(0, 255, 0); break;
                case LED_STATE_HUMIDITY_DRY:        currentColor = strip.Color(255, 255, 0); break;
                case LED_STATE_HUMIDITY_VERY_DRY:   currentColor = strip.Color(255, 0, 0); break;
            }

            // Hiển thị theo trạng thái mạng (Blink nếu mất mạng, sáng đứng nếu có mạng)
            if (xBinarySemaphoreInternet == NULL || uxSemaphoreGetCount(xBinarySemaphoreInternet) == 0) {
                // Mất mạng -> Nhấp nháy màu tương ứng với độ ẩm
                static bool toggle = false;
                toggle = !toggle;
                if (toggle) strip.setPixelColor(0, currentColor);
                else strip.setPixelColor(0, strip.Color(0, 0, 0));
                strip.show();
                vTaskDelay(pdMS_TO_TICKS(500)); 
            } else {
                // Có mạng -> Sáng đứng màu tương ứng
                strip.setPixelColor(0, currentColor);
                strip.show();
            }
        } else {
            // KHÔNG lấy được Semaphore -> WebServer đang đặt trạng thái OFF
            strip.clear();
            strip.show();
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Delay nhỏ để tránh chiếm dụng CPU
    }
}