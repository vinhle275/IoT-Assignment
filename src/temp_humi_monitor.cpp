// #include "temp_humi_monitor.h"
// DHT20 dht20;
// LiquidCrystal_I2C lcd(0x27,16,2);


// void temp_humi_monitor(void *pvParameters){

//     Wire.begin(11, 12);
//     Serial.begin(115200);
//     dht20.begin();

//     lcd.begin();
//     lcd.backlight(); 


//     SensorData_t currentData;

//     while (1){
//         /* code */
        
//         dht20.read();
//         // Reading temperature in Celsius
//         float temperature = dht20.getTemperature();
//         // Reading humidity
//         float humidity = dht20.getHumidity();

        

//         // Check if any reads failed and exit early
//         if (isnan(temperature) || isnan(humidity)) {
//             Serial.println("Failed to read from DHT sensor!");
//             temperature = humidity =  -1;
//             //return;
//         }else{
//             currentData.temperature = temperature;
//             currentData.humidity = humidity;
//         }


//         if (sensorQueue != NULL) {
//             xQueueOverwrite(sensorQueue, &currentData);
//         }

        
//         Serial.print("Humidity: ");
//         Serial.print(humidity);
//         Serial.print("%  Temperature: ");
//         Serial.print(temperature);
//         Serial.println("°C");





//         lcd.setCursor(0, 0);
//         lcd.print("Temp:");
//         lcd.print(temperature);
//         lcd.print("C");
//         lcd.setCursor(0, 1);
//         lcd.print("Humi:");
//         lcd.print(humidity);
//         lcd.print("%");
//         lcd.backlight(); 

        
//         vTaskDelay(5000);
//     }
    
// }




#include "temp_humi_monitor.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2);

// =========================================================
// CÁC HÀM XỬ LÝ PHỤ TRỢ RIÊNG CHO LCD
// =========================================================

/* 1. Hàm đánh giá dữ liệu để phân loại trạng thái FSM cho LCD */
static LCDState_t evaluate_lcd_state(float temp, float humi, bool &isTempAbnormal, bool &isHumiAbnormal) {
    // Đánh giá Nhiệt độ
    bool tempCritical = (temp >= TEMP_HOT || temp <= TEMP_COLD);
    bool tempWarning = ((temp >= TEMP_WARM && temp < TEMP_HOT) || 
                        (temp <= TEMP_COOL && temp > TEMP_COLD));
    isTempAbnormal = (tempCritical || tempWarning);

    // Đánh giá Độ ẩm
    bool humiCritical = (humi >= HUMIDITY_VERY_HUMID || humi <= HUMIDITY_VERY_DRY);
    bool humiWarning = ((humi >= HUMIDITY_HUMID && humi < HUMIDITY_VERY_HUMID) || 
                        (humi <= HUMIDITY_DRY && humi > HUMIDITY_VERY_DRY));
    isHumiAbnormal = (humiCritical || humiWarning);

    // Trả về trạng thái cao nhất
    if (tempCritical || humiCritical) return LCD_STATE_CRITICAL;
    if (tempWarning || humiWarning) return LCD_STATE_WARNING;
    return LCD_STATE_NORMAL;
}

/* 2. Hàm cập nhật nội dung văn bản hiển thị lên LCD */
static void update_lcd_display(float temp, float humi, bool isTempAbnormal, bool isHumiAbnormal) {
    // Serial.print("Humi: "); Serial.print(humi);
    // Serial.print("%  Temp: "); Serial.print(temp); Serial.println("C");

    lcd.clear(); // Xóa sạch màn hình trước khi in data mới

    // Dòng 1: In Nhiệt độ
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp, 2);
    lcd.print("C");
    if (isTempAbnormal) lcd.print("  SOS");

    // Dòng 2: In Độ ẩm
    lcd.setCursor(0, 1);
    lcd.print("Humi:");
    lcd.print(humi, 2);
    lcd.print("%");
    if (isHumiAbnormal) lcd.print("  SOS");
}

/* 3. Máy trạng thái điều khiển nhấp nháy đèn nền LCD */
static void handle_lcd_backlight_fsm(LCDState_t state, TickType_t currentTime) {
    static TickType_t lastLcdBlinkTime = 0;
    static bool lcdBacklightState = true;
    const TickType_t blinkInterval = pdMS_TO_TICKS(500);

    switch (state) {
        case LCD_STATE_CRITICAL:
            if ((currentTime - lastLcdBlinkTime) >= blinkInterval) {
                lcdBacklightState = !lcdBacklightState;
                if (lcdBacklightState) lcd.backlight();
                else lcd.noBacklight();
                lastLcdBlinkTime = currentTime;
            }
            break;
            
        case LCD_STATE_WARNING:
        case LCD_STATE_NORMAL:
            if (!lcdBacklightState) {
                lcd.backlight();
                lcdBacklightState = true;
            }
            break;
    }
}

// =========================================================
// TIẾN TRÌNH CHÍNH (MAIN TASK)
// =========================================================

void temp_humi_monitor(void *pvParameters) {
    // Khởi tạo ngoại vi
    Wire.begin(11, 12);
    dht20.begin();
    lcd.begin();
    lcd.backlight(); 

    SensorData_t currentData;
    LCDState_t currentState = LCD_STATE_NORMAL;
    
    TickType_t lastSensorReadTime = 0;
    const TickType_t sensorReadInterval = pdMS_TO_TICKS(5000); 

    float temperature = 0, humidity = 0;
    bool isTempAbnormal = false, isHumiAbnormal = false;

    while (1) {
        TickType_t currentTime = xTaskGetTickCount();

        // 1. CHU KỲ 5 GIÂY: Đọc cảm biến, lưu Queue và tính toán hiển thị
        if ((currentTime - lastSensorReadTime) >= sensorReadInterval || lastSensorReadTime == 0) {
            
            // --- ĐỌC CẢM BIẾN VÀ GHI QUEUE (Giữ nguyên trong Task) ---
            dht20.read();
            temperature = dht20.getTemperature();
            humidity = dht20.getHumidity();
            
            if (isnan(temperature) || isnan(humidity)) {
                Serial.println("Failed to read from DHT sensor!");
                temperature = humidity = -1;
            } else {
                currentData.temperature = temperature;
                currentData.humidity = humidity;
            }

            if (sensorQueue != NULL) {
                xQueueOverwrite(sensorQueue, &currentData);
            }


            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.print("%  Temperature: ");
            Serial.print(temperature);
            Serial.println("°C");
            // ----------------------------------------------------------
            
            // --- CẬP NHẬT TRẠNG THÁI VÀ HIỂN THỊ LCD ---
            currentState = evaluate_lcd_state(temperature, humidity, isTempAbnormal, isHumiAbnormal);
            update_lcd_display(temperature, humidity, isTempAbnormal, isHumiAbnormal);

            lastSensorReadTime = currentTime; 
        }

        // 2. CHU KỲ LIÊN TỤC: Cập nhật hiệu ứng chớp tắt đèn nền theo FSM
        handle_lcd_backlight_fsm(currentState, currentTime);

        // Delay 20ms nhường CPU cho các task khác
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}