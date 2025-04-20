#include "hal_interfaces.h"
#include "scale_config.h"
#include "driver/gpio.h" // Example ESP-IDF peripheral header
#include "esp_log.h"
// #include "HX711.h" // Example: If using an HX711 library

static const char *TAG = "HAL_LOADCELL";

// --- Variables specific to the HAL ---
// HX711 scale; // Example library object
static float current_calibration_factor = 1.0f; // Default, should be set
static long current_offset = 0L; // Tare offset
static bool is_initialized = false;

// For stability tracking within HAL
static float weight_buffer[STABLE_READING_COUNT];
static int buffer_idx = 0;
static int readings_count = 0;

void hal_LoadCell_Init(float calibration_factor) {
    ESP_LOGI(TAG, "Initializing Load Cell Driver...");
    // TODO: Initialize GPIO pins for DOUT and SCK
    // Example GPIO setup (ESP-IDF):
    // gpio_config_t io_conf = {};
    // io_conf.pin_bit_mask = (1ULL << LOADCELL_DOUT_PIN) | (1ULL << LOADCELL_SCK_PIN);
    // io_conf.mode = GPIO_MODE_INPUT; // Adjust modes based on HX711 or sensor needs
    // io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // gpio_config(&io_conf);

    // TODO: Initialize the sensor library (e.g., HX711)
    // scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    // scale.set_scale(calibration_factor);
    // scale.tare(); // Initial tare
    // current_offset = scale.get_offset();
    current_calibration_factor = calibration_factor;
    current_offset = LOADCELL_OFFSET; // Or get from initial tare

    memset(weight_buffer, 0, sizeof(weight_buffer));
    buffer_idx = 0;
    readings_count = 0;

    is_initialized = true;
    ESP_LOGI(TAG, "Load Cell Initialized. Cal Factor: %.2f, Offset: %ld", current_calibration_factor, current_offset);
    hal_LoadCell_Tare(); // Perform initial tare
}

LoadCellReading_t hal_LoadCell_Read(float max_weight, float stable_threshold, int stable_count_needed) {
    LoadCellReading_t result = {0};
    if (!is_initialized) {
        ESP_LOGE(TAG, "HAL LoadCell not initialized!");
        result.is_overload = true; // Indicate error
        return result;
    }

    // TODO: Implement actual reading from sensor/library
    // Placeholder implementation:
    // if (scale.is_ready()) {
    //    result.raw_value = scale.read(); // Read raw value
    //    result.weight_grams = scale.get_units(1); // Read average of 1 reading with current scale factor
    // } else {
    //    ESP_LOGW(TAG,"Scale not ready");
    //    result.weight_grams = 0; // Or last known good value?
    //    result.raw_value = 0;
    // }

    // --- Placeholder Data (REMOVE THIS IN REAL IMPLEMENTATION) ---
    static float dummy_weight = 50.0f;
    static int stable_sim_count = 0;
    dummy_weight += (float)(rand() % 11 - 5) / 10.0f; // Simulate small fluctuations
    if (dummy_weight < 0) dummy_weight = 0;
    if (dummy_weight > max_weight * 1.1f) dummy_weight = max_weight * 1.1f; // Simulate overload slightly above threshold
    result.weight_grams = dummy_weight;
    result.raw_value = (long)((result.weight_grams * current_calibration_factor) + current_offset);
    // --- End Placeholder Data ---


    // --- Overload Check ---
    // Usually check based on weight, but raw might be better if scale factor changes
    if (result.weight_grams > max_weight) { // Check against capacity
         result.is_overload = true;
         result.is_stable = false; // Not stable if overloaded
         readings_count = 0; // Reset stability count
         ESP_LOGW(TAG, "Overload detected: %.2f g", result.weight_grams);
         return result; // Return early if overloaded
    } else {
         result.is_overload = false;
    }

    // --- Stability Check ---
    // Store current reading in circular buffer
    weight_buffer[buffer_idx] = result.weight_grams;
    buffer_idx = (buffer_idx + 1) % stable_count_needed;
    if (readings_count < stable_count_needed) {
        readings_count++;
    }

    result.is_stable = false; // Assume not stable unless proven otherwise
    if (readings_count >= stable_count_needed) {
        float min_w = weight_buffer[0];
        float max_w = weight_buffer[0];
        for (int i = 1; i < stable_count_needed; i++) {
            if (weight_buffer[i] < min_w) min_w = weight_buffer[i];
            if (weight_buffer[i] > max_w) max_w = weight_buffer[i];
        }
        // Check if the difference between max and min is within the threshold
        if (fabsf(max_w - min_w) <= stable_threshold) {
            result.is_stable = true;
        }
    }

     // ESP_LOGD(TAG, "Read: %.2fg, Stable: %d", result.weight_grams, result.is_stable);
    return result;
}

void hal_LoadCell_Tare(void) {
     if (!is_initialized) return;
     ESP_LOGI(TAG, "Performing Tare...");
     // TODO: Implement Tare using sensor library
     // scale.tare(10); // Example: average 10 readings for tare
     // current_offset = scale.get_offset();

     // --- Placeholder ---
     current_offset += (long)(dummy_weight * current_calibration_factor); // Simulate offset change
     dummy_weight = 0.0f; // Reset dummy weight after tare
     // --- End Placeholder ---

     readings_count = 0; // Reset stability buffer after tare
     ESP_LOGI(TAG, "Tare complete. New Offset: %ld", current_offset);
}

void hal_LoadCell_SetCalibrationFactor(float factor) {
     if (!is_initialized || factor == 0) return;
     current_calibration_factor = factor;
     // TODO: Update scale factor in sensor library
     // scale.set_scale(factor);
     ESP_LOGI(TAG, "Calibration factor set to: %.2f", factor);
}

float hal_LoadCell_GetCalibrationFactor(void){
     return current_calibration_factor;
}

long hal_LoadCell_GetOffset(void){
     // TODO: Get offset from library if possible
     // return scale.get_offset();
     return current_offset; // Return HAL's tracked offset
}
