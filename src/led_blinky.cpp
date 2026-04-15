#include "led_blinky.h"


void led_blinky(void *pvParameters) {
    //Serial.begin(115200);
    pinMode(LED_GPIO, OUTPUT);
    
    SensorData_t receivedData;
    
    LEDBlinkyState_t currentState = LED_STATE_NORMAL_TEMP;
    

    TickType_t blinkInterval = pdMS_TO_TICKS(1000); 
    TickType_t lastToggleTime = xTaskGetTickCount();
    bool ledState = false;

    while (1) {
        if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS){
            if (receivedData.temperature >= TEMP_HOT) {
                currentState = LED_STATE_TEMP_HOT;
            } 
            else if (receivedData.temperature >= TEMP_WARM && receivedData.temperature < TEMP_HOT) {
                currentState = LED_STATE_TEMP_WARM;
                //Serial.print("Warm");
            } 
            else if (receivedData.temperature <= TEMP_COLD) {
                currentState = LED_STATE_TEMP_COLD;
            } 
            else if (receivedData.temperature <= TEMP_COOL && receivedData.temperature > TEMP_COLD) {
                currentState = LED_STATE_TEMP_COOL;
            } 
            else {
                currentState = LED_STATE_NORMAL_TEMP; 
            }
        }





        switch (currentState) {
            case LED_STATE_TEMP_HOT:  
                blinkInterval = pdMS_TO_TICKS(100);  
                break;
            case LED_STATE_TEMP_WARM: 
                blinkInterval = pdMS_TO_TICKS(300);  
                break;
            case LED_STATE_NORMAL_TEMP:    
                blinkInterval = pdMS_TO_TICKS(1000); 
                break;
            case LED_STATE_TEMP_COOL: 
                blinkInterval = pdMS_TO_TICKS(2000); 
                break;
            case LED_STATE_TEMP_COLD: 
                blinkInterval = pdMS_TO_TICKS(3000); 
                break;
        }

        // if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
        //     ledState = !ledState;
        //     digitalWrite(LED_GPIO, ledState);
        //     lastToggleTime = xTaskGetTickCount();
        // }

        if (xSemaphoreTake(xSemaphoreLedControl, 0) == pdTRUE) {
            
            // Lấy được Semaphore -> Trả lại ngay để giữ trạng thái ON cho vòng lặp sau
            xSemaphoreGive(xSemaphoreLedControl); 
            
            // Xử lý chớp tắt LED bình thường
            if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
                ledState = !ledState;
                digitalWrite(LED_GPIO, ledState);
                lastToggleTime = xTaskGetTickCount();
            }
        } else {
            // Không lấy được Semaphore -> Lệnh OFF đang có hiệu lực
            ledState = false;
            digitalWrite(LED_GPIO, LOW); 
            lastToggleTime = xTaskGetTickCount(); // Cập nhật thời gian
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}