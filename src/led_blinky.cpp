#include "led_blinky.h"

// void led_blinky(void *pvParameters){
//   pinMode(LED_GPIO, OUTPUT);
  
//   while(1) {                        
//     digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
//     vTaskDelay(500);
//     digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
//     vTaskDelay(500);
//   }
// }






void led_blinky(void *pvParameters) {
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

        if ((xTaskGetTickCount() - lastToggleTime) >= blinkInterval) {
            ledState = !ledState;
            digitalWrite(LED_GPIO, ledState);
            lastToggleTime = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}