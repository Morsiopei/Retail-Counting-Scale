#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h" // Use ESP-IDF logging

#include "scale_config.h"
#include "hal_interfaces.h"
#include "scale_logic.h"
#include "ui_manager.h"
#include "comms_manager.h"

// Task Function Prototypes
void sensor_task(void *pvParameters);
void ui_task(void *pvParameters);
void comms_task(void *pvParameters);

// Shared scale state
static ScaleState_t scale_state;

// Logging Tag
static const char *TAG = "MAIN";

// --- Main Application Entry Point ---
void app_main(void) {
    ESP_LOGI(TAG, "Starting Counting Scale System Firmware");

    // --- Initialize HAL ---
    ESP_LOGI(TAG, "Initializing Hardware Abstraction Layer...");
    hal_Storage_Init();     // Init storage first to load config early
    hal_LoadCell_Init(LOADCELL_CALIBRATION_FACTOR); // Pass initial calibration factor
    hal_Display_Init();
    hal_Buttons_Init();
    hal_Wifi_Init();        // Initialize WiFi hardware/stack

    // --- Initialize Core Logic & Modules ---
    ESP_LOGI(TAG, "Initializing Logic and Managers...");
    ScaleLogic_Init(&scale_state);
    ScaleLogic_LoadConfig(&scale_state); // Attempt to load saved avg item weight
    UIManager_Init(&scale_state);
    CommsManager_Init();

    // Display initial message
    hal_Display_Clear();
    hal_Display_SetCursor(0, 0);
    hal_Display_Print("Initializing...");
    hal_Display_Update();
    hal_System_DelayMs(1000); // Brief pause

    // --- Create RTOS Tasks ---
    // Note: Adjust stack sizes based on actual usage and complexity
    ESP_LOGI(TAG, "Creating RTOS Tasks...");
    xTaskCreate(sensor_task,        // Task function
                "SensorTask",       // Task name
                4096,               // Stack size (bytes)
                (void*)&scale_state,// Parameter to pass
                5,                  // Priority (higher number = higher priority)
                NULL);              // Task handle (optional)

    xTaskCreate(ui_task, "UITask", 2048, (void*)&scale_state, 4, NULL);

    xTaskCreate(comms_task, "CommsTask", 4096, (void*)&scale_state, 3, NULL);


    ESP_LOGI(TAG, "Initialization Complete. Tasks Started.");
    // The ESP-IDF `app_main` function returns, and the FreeRTOS scheduler runs the created tasks.
}


// --- Task Implementations (could be in separate files) ---

// Sensor Task: Reads load cell and updates shared state
void sensor_task(void *pvParameters) {
    ScaleState_t *state = (ScaleState_t *)pvParameters;
    LoadCellReading_t current_reading;
    ESP_LOGI(TAG, "Sensor Task Started.");

    while (1) {
        // Read from HAL
        current_reading = hal_LoadCell_Read(MAX_WEIGHT_CAPACITY_G,
                                            STABLE_READING_THRESHOLD_G,
                                            STABLE_READING_COUNT);

        // --- Critical Section (Example using simple approach, consider mutex for complex state) ---
        // If using mutex: xSemaphoreTake(state->mutex, portMAX_DELAY);
        ScaleLogic_Update(state, ¤t_reading);
        // If using mutex: xSemaphoreGive(state->mutex);
        // --- End Critical Section ---

        // Log raw value occasionally for debugging (optional)
        // ESP_LOGD(TAG, "Raw Sensor: %ld, Weight: %.2fg, Stable: %d",
        //         current_reading.raw_value, state->current_weight_g, state->is_stable);

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_INTERVAL_MS));
    }
}

// UI Task: Handles button input and updates the display
void ui_task(void *pvParameters) {
    ScaleState_t *state = (ScaleState_t *)pvParameters;
    ButtonEvent_t event;
    ESP_LOGI(TAG, "UI Task Started.");

    while (1) {
        // Check for button input
        event = hal_Buttons_Read();
        if (event != BUTTON_NONE) {
             ESP_LOGD(TAG, "Button Event: %d", event);
            // --- Critical Section (Example) ---
            // xSemaphoreTake(state->mutex, portMAX_DELAY);
            UIManager_HandleInput(state, event); // HandleInput modifies state
            // xSemaphoreGive(state->mutex);
            // --- End Critical Section ---
        }

        // Update the display based on the current state
        // --- Critical Section (Example - Read Only) ---
        // xSemaphoreTake(state->mutex, portMAX_DELAY);
        UIManager_UpdateDisplay(state);
        // xSemaphoreGive(state->mutex);
        // --- End Critical Section ---


        vTaskDelay(pdMS_TO_TICKS(UI_TASK_INTERVAL_MS));
    }
}

// Comms Task: Manages WiFi connection and sends data periodically
void comms_task(void *pvParameters) {
    ScaleState_t *state = (ScaleState_t *)pvParameters;
     ESP_LOGI(TAG, "Comms Task Started.");

    while (1) {
        // Run the communications state machine / periodic checks
        CommsManager_RunPeriodic(); // Handles connection logic

        // If connected, try sending data
        if (CommsManager_GetCurrentState() == COMMS_STATE_CONNECTED) {
            // --- Critical Section (Example - Read Only) ---
            // xSemaphoreTake(state->mutex, portMAX_DELAY);
            CommsManager_SendData(state);
            // xSemaphoreGive(state->mutex);
            // --- End Critical Section ---
        }

        vTaskDelay(pdMS_TO_TICKS(COMMS_TASK_INTERVAL_MS)); // Run less frequently
    }
}
