#include "lcd.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);



LCDState_t determineNextState(SensorData_t currentData) {
    bool hasWifi = false;
    
    // Kiểm tra Wi-Fi
    if (xBinarySemaphoreInternet != NULL) {
        if (xSemaphoreTake(xBinarySemaphoreInternet, 0) == pdTRUE) {
            hasWifi = true;
            xSemaphoreGive(xBinarySemaphoreInternet); // Lấy xong phải trả lại ngay
        }
    }

    if (!hasWifi) {
        return LCD_STATE_NO_WIFI;
    }

    if (currentData.temperature >= TEMP_HOT || currentData.humidity >= HUMIDITY_VERY_HUMID) {
        return LCD_STATE_CRITICAL;
    } else if (currentData.temperature >= TEMP_WARM || currentData.humidity >= HUMIDITY_HUMID) {
        return LCD_STATE_WARNING;
    } else {
        return LCD_STATE_NORMAL;
    }
}



void executeState(LCDState_t state, SensorData_t data, LiquidCrystal_I2C &lcd, TickType_t &lastBlinkTime, bool &isLcdOn) {
    TickType_t currentTime = xTaskGetTickCount();
    
    switch (state) {
        case LCD_STATE_NO_WIFI:
            lcd.setCursor(4, 0);
            lcd.print("NO WIFI      "); 
            lcd.setCursor(0, 1);
            lcd.print("                "); 
            lcd.backlight(); 
            break;

        case LCD_STATE_NORMAL:
            lcd.setCursor(0, 0);
            lcd.printf("Temp: %.1f C   ", data.temperature);
            lcd.setCursor(0, 1);
            lcd.printf("Humi: %.1f %%  ", data.humidity);
            lcd.backlight(); 
            break;

        case LCD_STATE_WARNING:
            lcd.setCursor(0, 0);
            lcd.printf("Temp: %.1f C   ", data.temperature);
            lcd.setCursor(0, 1);
            lcd.printf("Humi: %.1f %%  ", data.humidity);
            
            if ((currentTime - lastBlinkTime) >= (1000 / portTICK_PERIOD_MS)) {
                isLcdOn = !isLcdOn;
                isLcdOn ? lcd.backlight() : lcd.noBacklight();
                lastBlinkTime = currentTime;
            }
            break;

        case LCD_STATE_CRITICAL:
            lcd.setCursor(0, 0);
            lcd.printf("Temp: %.1f C   ", data.temperature);
            lcd.setCursor(0, 1);
            lcd.printf("Humi: %.1f %%  ", data.humidity);
            
            if ((currentTime - lastBlinkTime) >= (200 / portTICK_PERIOD_MS)) {
                isLcdOn = !isLcdOn;
                isLcdOn ? lcd.backlight() : lcd.noBacklight();
                lastBlinkTime = currentTime;
            }
            break;
    }
}


void task_lcd(void *pvParameters) {
    
    lcd.begin();
    lcd.backlight();


    SensorData_t currentData = {0, 0};
    LCDState_t currentState = LCD_STATE_NO_WIFI;
    LCDState_t lastState = LCD_STATE_NO_WIFI; 
    
    TickType_t lastBlinkTime = xTaskGetTickCount();
    bool isLcdOn = true;

    while (1) {
        if (sensorQueue != NULL) {
            xQueuePeek(sensorQueue, &currentData, 0);
        }

        currentState = determineNextState(currentData);

        // Bước Chuyển tiếp: Chỉ clear màn hình khi trạng thái bị thay đổi
        if (currentState != lastState) {
            lcd.clear();
            isLcdOn = true;
            lcd.backlight(); 
            lastState = currentState;
        }

        // Bước 2: Truyền địa chỉ các biến cục bộ vào hàm thực thi
        executeState(currentState, currentData, lcd, lastBlinkTime, isLcdOn);


        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}