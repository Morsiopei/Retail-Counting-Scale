#include "ui_manager.h"
#include "hal_interfaces.h"
#include "scale_config.h" // For display dimensions etc.
#include "scale_logic.h" // For direct logic calls if needed
#include <stdio.h> // For snprintf
#include "esp_log.h"

static const char *TAG = "UI_MANAGER";

void UIManager_Init(const ScaleState_t *initial_state) {
    // Initial display update can happen here or in the first run of ui_task
    hal_Display_Clear();
    hal_Display_SetCursor(10, 10);
    hal_Display_Print("Scale Ready");
    // Maybe display initial mode based on initial_state->current_mode
    hal_Display_Update();
    ESP_LOGI(TAG, "UI Manager Initialized.");
}

void UIManager_UpdateDisplay(const ScaleState_t *state) {
    char buffer[64]; // Buffer for formatting strings

    hal_Display_Clear();

    // Line 1: Weight (always show)
    hal_Display_SetCursor(0, 0);
    if (state->current_mode == MODE_ERROR && state->is_overload) {
         hal_Display_Printf("OVERLOAD!");
    } else {
        // Format weight with 1 decimal place, right-aligned maybe?
        snprintf(buffer, sizeof(buffer), "%.1f g", state->current_weight_g);
        hal_Display_Print(buffer); // Basic left alignment for now
    }


    // Line 2: Item Count or Mode Indicator
    hal_Display_SetCursor(0, 16); // Adjust Y coordinate based on font size
    if (state->current_mode == MODE_COUNTING) {
         if (state->average_item_weight_g < 0.001f) {
              hal_Display_Print("Set Sample Wt");
         } else {
            snprintf(buffer, sizeof(buffer), "Count: %ld", state->item_count);
            hal_Display_Print(buffer);
         }
    } else if (state->current_mode == MODE_WEIGHING) {
        hal_Display_Print("Mode: Weigh");
    } else if (state->current_mode == MODE_ERROR) {
         hal_Display_Print("Mode: Error");
    } else if (state->current_mode == MODE_SET_SAMPLE) {
         hal_Display_Print("Setting Sample...");
    }


    // Line 3: Stability and Status Message
    hal_Display_SetCursor(0, 32); // Adjust Y
    snprintf(buffer, sizeof(buffer), "%s [%s]",
             state->is_stable ? "Stable" : " ...  ",
             state->status_message); // Show status from logic
     hal_Display_Print(buffer);


    // Line 4: Average Item Weight (if in counting mode) or WiFi status
    hal_Display_SetCursor(0, 48); // Adjust Y
    if (state->current_mode == MODE_COUNTING && state->average_item_weight_g > 0.001f) {
         snprintf(buffer, sizeof(buffer), "Avg: %.3fg", state->average_item_weight_g);
         hal_Display_Print(buffer);
    } else {
         // Optionally show WiFi status from CommsManager state? Requires access.
         // CommsState_t comms_state = CommsManager_GetCurrentState(); // Need getter
         // hal_Display_Printf("WiFi: %s", comms_state == COMMS_STATE_CONNECTED ? "OK" : "---");
          hal_Display_Print(""); // Placeholder
    }


    // Send buffer to the actual display hardware
    hal_Display_Update();
}


void UIManager_HandleInput(ScaleState_t *state, ButtonEvent_t event) {
    switch (event) {
        case BUTTON_TARE_PRESS:
            ESP_LOGI(TAG, "Tare button pressed.");
            ScaleLogic_RequestTare(state);
            break;

        case BUTTON_SAMPLE_PRESS:
             ESP_LOGI(TAG, "Sample button pressed.");
            ScaleLogic_RequestSetSample(state);
            break;

        case BUTTON_MODE_PRESS:
            ESP_LOGI(TAG, "Mode button pressed.");
            ScaleLogic_RequestToggleMode(state);
            break;

        // Handle HOLD events if implemented in HAL and needed
        // case BUTTON_TARE_HOLD:
        //    // e.g., Enter calibration mode?
        //    break;

        case BUTTON_NONE:
        default:
            // No action needed
            break;
    }
    // State changes are handled within the ScaleLogic functions called above.
    // The display will update on the next call to UIManager_UpdateDisplay.
}
