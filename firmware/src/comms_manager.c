#include "comms_manager.h"
#include "hal_interfaces.h"
#include "scale_config.h"
#include <stdio.h> // For snprintf
#include <string.h>
#include "esp_log.h"

static const char *TAG = "COMMS_MANAGER";
static CommsState_t current_comms_state = COMMS_STATE_DISCONNECTED;
static uint64_t last_connect_attempt_ms = 0;

void CommsManager_Init(void) {
    // HAL WiFi Init is usually done in main.c
    current_comms_state = COMMS_STATE_DISCONNECTED;
    ESP_LOGI(TAG, "Comms Manager Initialized.");
    // Immediately try to connect on startup
    CommsManager_Connect();
}

CommsState_t CommsManager_GetCurrentState(void) {
    return current_comms_state;
}

void CommsManager_Connect(void) {
    if (current_comms_state == COMMS_STATE_CONNECTING || current_comms_state == COMMS_STATE_CONNECTED) {
        return; // Already connected or connecting
    }

    ESP_LOGI(TAG, "Attempting WiFi connection...");
    current_comms_state = COMMS_STATE_CONNECTING;
    last_connect_attempt_ms = hal_System_GetTickMs();

    // Asynchronous connect call (if HAL supports it) or blocking call here
    // This example uses a blocking call for simplicity within RunPeriodic
    // bool connected = hal_Wifi_Connect(WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECT_TIMEOUT_MS);
    // if (connected) {
    //     current_comms_state = COMMS_STATE_CONNECTED;
    //     ESP_LOGI(TAG, "WiFi Connected.");
    // } else {
    //     current_comms_state = COMMS_STATE_ERROR; // Or DISCONNECTED to retry
    //     ESP_LOGE(TAG, "WiFi Connection Failed.");
    // }
}

void CommsManager_RunPeriodic(void) {
    uint64_t now = hal_System_GetTickMs();

    switch (current_comms_state) {
        case COMMS_STATE_DISCONNECTED:
            // Retry connection periodically
            if (now - last_connect_attempt_ms > 30000) { // Retry every 30 seconds
                 CommsManager_Connect(); // Initiate connection attempt
            }
            break;

        case COMMS_STATE_CONNECTING:
             // If connect was blocking, this state might be short.
             // If it was async, check status here.
             // For simplicity, assume connect is handled within this periodic check
             if (now - last_connect_attempt_ms > WIFI_CONNECT_TIMEOUT_MS) {
                  ESP_LOGE(TAG, "WiFi connection timed out.");
                  hal_Wifi_Disconnect(); // Ensure cleanup
                  current_comms_state = COMMS_STATE_DISCONNECTED; // Go back to disconnected to retry later
                  last_connect_attempt_ms = now; // Reset timer
             } else {
                 // Re-check connection status explicitly
                 if (hal_Wifi_IsConnected()) {
                      ESP_LOGI(TAG, "WiFi Connected.");
                      current_comms_state = COMMS_STATE_CONNECTED;
                 }
                 // else, remain in CONNECTING state
             }
            break;

        case COMMS_STATE_CONNECTED:
            // Check if still connected
            if (!hal_Wifi_IsConnected()) {
                ESP_LOGW(TAG, "WiFi connection lost.");
                current_comms_state = COMMS_STATE_DISCONNECTED;
                last_connect_attempt_ms = now; // Reset timer for retry
            }
            // Data sending is handled by the comms_task calling CommsManager_SendData
            break;

        case COMMS_STATE_SENDING:
            // This state might be used if sending is asynchronous
            // Check status of ongoing send operation
            // Transition back to CONNECTED or ERROR
             ESP_LOGD(TAG, "Currently sending data..."); // Placeholder log
            break;

        case COMMS_STATE_ERROR:
            // Error state, maybe try reconnecting after a longer delay
             if (now - last_connect_attempt_ms > 60000) { // Retry every 60 seconds from error
                 ESP_LOGI(TAG, "Retrying connection after error...");
                 current_comms_state = COMMS_STATE_DISCONNECTED; // Go back to disconnected
                 CommsManager_Connect();
            }
            break;
    }
}


void CommsManager_SendData(const ScaleState_t *state) {
    if (current_comms_state != COMMS_STATE_CONNECTED) {
        ESP_LOGW(TAG, "Cannot send data, not connected.");
        return;
    }

    char payload[256]; // Adjust size as needed
    char response_buffer[128]; // Adjust size

    // Format data as JSON payload
    // Note: Using snprintf is basic. A dedicated JSON library (like cJSON) is better for complex data.
    snprintf(payload, sizeof(payload),
             "{\"device_id\":\"%s\", \"timestamp\":\"%llu\", \"weight_grams\":%.2f, "
             "\"item_count\":%ld, \"is_stable\":%s, \"is_overload\":%s, "
             "\"average_item_weight\":%.3f, \"mode\":\"%s\"}",
             DEVICE_ID,
             hal_System_GetTickMs(), // Use system ticks as a simple timestamp proxy
             state->current_weight_g,
             state->item_count,
             state->is_stable ? "true" : "false",
             state->is_overload ? "true" : "false",
             state->average_item_weight_g,
             (state->current_mode == MODE_COUNTING) ? "COUNTING" : ((state->current_mode == MODE_WEIGHING) ? "WEIGHING" : "ERROR")
    );

    ESP_LOGI(TAG, "Sending data: %s", payload);
    current_comms_state = COMMS_STATE_SENDING; // Indicate sending started

    // Make the HTTP POST request via HAL
    int http_status = hal_Wifi_HttpPost(API_ENDPOINT_URL, payload, response_buffer, sizeof(response_buffer), API_REQUEST_TIMEOUT_MS);

    if (http_status >= 200 && http_status < 300) {
        ESP_LOGI(TAG, "Data sent successfully. Status: %d. Response: %s", http_status, response_buffer);
        current_comms_state = COMMS_STATE_CONNECTED; // Return to connected state
    } else {
        ESP_LOGE(TAG, "Failed to send data. HTTP Status: %d", http_status);
        current_comms_state = COMMS_STATE_CONNECTED; // Could go to ERROR, but maybe just retry next time
        // If http_status < 0, it indicates a connection/network error from HAL
        if (http_status < 0) {
             ESP_LOGE(TAG, "Network error during send. Checking connection...");
             current_comms_state = COMMS_STATE_DISCONNECTED; // Assume connection issue
             last_connect_attempt_ms = hal_System_GetTickMs();
        }
    }
}
