#include "tinyml.h"

namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model_ptr = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;

    constexpr int kTensorArenaSize = 16 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
}

// StandardScaler values from ESP32_Config.txt
constexpr float TEMP_MEAN = 18.6480f;
constexpr float TEMP_STD  = 16.6074f;
constexpr float HUMI_MEAN = 68.5105f;
constexpr float HUMI_STD  = 20.2033f;

// Labels must match training order
const char *weather_labels[4] = {
    "Cloudy",
    "Rainy",
    "Snowy",
    "Sunny"
};

void setupTinyML()
{
    Serial.println("Initializing TensorFlow Lite Micro...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Check the exact array name inside toanvinh_model.h
    // If your generated file uses another name, replace it here.
    model_ptr = tflite::GetModel(toanvinh_model_tflite);

    if (model_ptr->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model schema mismatch!");
        Serial.println("Model schema mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model_ptr,
        resolver,
        tensor_arena,
        kTensorArenaSize,
        error_reporter);

    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        Serial.println("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized successfully.");
}

void tiny_ml_task(void *pvParameters)
{
    setupTinyML();

    while (1)
    {
        SensorData_t receivedData;

        if (xQueuePeek(sensorQueue, &receivedData, 0) == pdPASS)
        {
            float raw_temp = receivedData.temperature;
            float raw_humi = receivedData.humidity;

            // Scale input exactly like training
            float temp_scaled = (raw_temp - TEMP_MEAN) / TEMP_STD;
            float humi_scaled = (raw_humi - HUMI_MEAN) / HUMI_STD;

            input->data.f[0] = temp_scaled;
            input->data.f[1] = humi_scaled;

            if (interpreter->Invoke() != kTfLiteOk)
            {
                error_reporter->Report("Invoke failed");
                Serial.println("Invoke failed");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            int best_idx = 0;
            float best_score = output->data.f[0];

            Serial.println("===== TinyML Weather Prediction =====");
            Serial.print("Raw Temperature: ");
            Serial.println(raw_temp);
            Serial.print("Raw Humidity: ");
            Serial.println(raw_humi);

            Serial.print("Scaled Temperature: ");
            Serial.println(temp_scaled, 6);
            Serial.print("Scaled Humidity: ");
            Serial.println(humi_scaled, 6);

            Serial.println("Class scores:");
            for (int i = 0; i < 4; i++)
            {
                Serial.print(weather_labels[i]);
                Serial.print(": ");
                Serial.println(output->data.f[i], 6);

                if (output->data.f[i] > best_score)
                {
                    best_score = output->data.f[i];
                    best_idx = i;
                }
            }

            Serial.print("Predicted weather: ");
            Serial.println(weather_labels[best_idx]);
            Serial.println("====================================");
        }
        else
        {
            Serial.println("tiny_ml_task: No sensor data available in queue.");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}