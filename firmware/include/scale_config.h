#ifndef SCALE_CONFIG_H
#define SCALE_CONFIG_H

// --- Hardware Pins (Example for ESP32 - REPLACE with actual pins) ---
#define LOADCELL_DOUT_PIN   GPIO_NUM_19
#define LOADCELL_SCK_PIN    GPIO_NUM_18
#define DISPLAY_SDA_PIN     GPIO_NUM_21
#define DISPLAY_SCL_PIN     GPIO_NUM_22
#define DISPLAY_RST_PIN     GPIO_NUM_17 // Optional Reset Pin for some displays
#define BUTTON_TARE_PIN     GPIO_NUM_15
#define BUTTON_SAMPLE_PIN   GPIO_NUM_4
#define BUTTON_MODE_PIN     GPIO_NUM_5  // Example extra button

// --- Load Cell Configuration ---
#define LOADCELL_CALIBRATION_FACTOR 425.0f // IMPORTANT: Calibrate this value!
#define LOADCELL_OFFSET             0L     // Will be determined by tare()

// --- Operational Parameters ---
#define STABLE_READING_THRESHOLD_G  0.5f // Max weight deviation in grams for stability
#define STABLE_READING_COUNT        5    // How many consecutive readings must be within threshold
#define DEFAULT_SENSITIVITY         1.0f // Future use?
#define MAX_WEIGHT_CAPACITY_G       5000.0f // Max weight in grams
#define OVERLOAD_THRESHOLD_G        (MAX_WEIGHT_CAPACITY_G * 1.05f) // 5% overload margin
#define MIN_SAMPLE_WEIGHT_G         1.0f // Minimum weight to set as a sample

// --- Communication ---
// WARNING: Avoid hardcoding credentials in production. Use secure provisioning.
#define WIFI_SSID           "YourNetworkSSID"
#define WIFI_PASSWORD       "YourNetworkPassword"
#define WIFI_CONNECT_TIMEOUT_MS 30000 // 30 seconds
#define API_ENDPOINT_URL    "http://your_backend_ip_or_domain:5000/api/v1/reading"
#define API_REQUEST_TIMEOUT_MS 5000 // 5 seconds
#define DEVICE_ID           "SCALE_SN_12345" // Unique ID for this scale

// --- Timing ---
#define SENSOR_TASK_INTERVAL_MS 50   // Read sensor this often
#define UI_TASK_INTERVAL_MS     100  // Update UI and check buttons this often
#define COMMS_TASK_INTERVAL_MS  15000 // Send data to backend this often (15s)

// --- UI ---
#define DISPLAY_WIDTH        128 // Example for OLED
#define DISPLAY_HEIGHT       64  // Example for OLED
#define DISPLAY_ADDRESS      0x3C // Example I2C address

// --- Storage ---
#define NVS_NAMESPACE "scale_cfg" // Non-Volatile Storage namespace
#define NVS_KEY_SAMPLE_WT "sample_wt" // Key for storing average item weight

#endif // SCALE_CONFIG_H
