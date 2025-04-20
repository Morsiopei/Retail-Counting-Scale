#ifndef HAL_INTERFACES_H
#define HAL_INTERFACES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h> // For size_t

// --- Load Cell Interface ---
typedef struct {
    float weight_grams; // Tare-adjusted weight
    bool is_stable;
    bool is_overload; // Based on raw reading potentially
    long raw_value;    // Raw ADC value (for diagnostics/calibration)
} LoadCellReading_t;

void hal_LoadCell_Init(float calibration_factor);
LoadCellReading_t hal_LoadCell_Read(float max_weight, float stable_threshold, int stable_count);
void hal_LoadCell_Tare(void); // Sets the zero offset
void hal_LoadCell_SetCalibrationFactor(float factor);
float hal_LoadCell_GetCalibrationFactor(void);
long hal_LoadCell_GetOffset(void); // Get current tare offset value

// --- Display Interface ---
void hal_Display_Init(void);
void hal_Display_Clear(void);
void hal_Display_SetCursor(int x, int y);
void hal_Display_Print(const char* text);
void hal_Display_Printf(const char* format, ...); // Formatted print
void hal_Display_DrawLine(int x0, int y0, int x1, int y1); // Example graphics
void hal_Display_Update(void); // Send buffer to display (if needed)

// --- Button Interface ---
typedef enum {
    BUTTON_NONE = 0,
    BUTTON_TARE_PRESS,
    BUTTON_TARE_HOLD, // Optional
    BUTTON_SAMPLE_PRESS,
    BUTTON_SAMPLE_HOLD, // Optional
    BUTTON_MODE_PRESS,
    // Add more as needed
} ButtonEvent_t;

void hal_Buttons_Init(void);
ButtonEvent_t hal_Buttons_Read(void); // Returns the current button event (non-blocking check)

// --- WiFi Interface ---
void hal_Wifi_Init(void);
bool hal_Wifi_Connect(const char* ssid, const char* password, uint32_t timeout_ms);
bool hal_Wifi_IsConnected(void);
void hal_Wifi_Disconnect(void);
// Returns HTTP status code, response stored in buffer. Returns < 0 on connection error.
int hal_Wifi_HttpPost(const char* url, const char* payload, char* response_buffer, size_t buffer_size, uint32_t timeout_ms);

// --- Storage Interface (Example using NVS - Non-Volatile Storage) ---
void hal_Storage_Init(void);
bool hal_Storage_Save_Float(const char* namespace, const char* key, float value);
bool hal_Storage_Load_Float(const char* namespace, const char* key, float* value);
// Add Save/Load functions for other types (int, string, blob) as needed
bool hal_Storage_Erase_Key(const char* namespace, const char* key);
bool hal_Storage_Erase_Namespace(const char* namespace);

// --- System Interface ---
void hal_System_DelayMs(uint32_t ms);
uint64_t hal_System_GetTickMs(void); // Get system uptime in ms
void hal_System_Reboot(void);

#endif // HAL_INTERFACES_H
