#include "scale_logic.h"
#include "scale_config.h"
#include "hal_interfaces.h"
#include <stdio.h> // For snprintf
#include <string.h> // For strcpy, memset
#include <math.h>   // For roundf, fabsf
#include "esp_log.h"

static const char *TAG = "SCALE_LOGIC";

// Helper to update status message safely
static void set_status(ScaleState_t *state, const char *message) {
    strncpy(state->status_message, message, sizeof(state->status_message) - 1);
    state->status_message[sizeof(state->status_message) - 1] = '\0'; // Ensure null termination
}

void ScaleLogic_Init(ScaleState_t *state) {
    memset(state, 0, sizeof(ScaleState_t)); // Clear the state structure
    state->current_mode = MODE_WEIGHING;
    state->average_item_weight_g = 0.0f; // Will be loaded from NVS if possible
    set_status(state, "Initializing");
    // Initialize stability tracking
    state->stable_counter = 0;
    state->weight_history_index = 0;
    // If using mutex: state->mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Scale Logic Initialized.");
}

void ScaleLogic_LoadConfig(ScaleState_t *state) {
    float loaded_weight = 0.0f;
    if (hal_Storage_Load_Float(NVS_NAMESPACE, NVS_KEY_SAMPLE_WT, &loaded_weight)) {
        if (loaded_weight > 0.001f) { // Basic validity check
            state->average_item_weight_g = loaded_weight;
            // Automatically switch to counting mode if a valid weight was loaded
            state->current_mode = MODE_COUNTING;
            ESP_LOGI(TAG, "Loaded average item weight: %.3f g", state->average_item_weight_g);
        } else {
            ESP_LOGI(TAG, "Loaded average item weight is zero or invalid, staying in weighing mode.");
            state->current_mode = MODE_WEIGHING; // Ensure weighing mode
        }
    } else {
        ESP_LOGW(TAG, "Could not load average item weight from NVS. Starting in weighing mode.");
        state->current_mode = MODE_WEIGHING; // Ensure weighing mode
    }
     set_status(state, state->current_mode == MODE_COUNTING ? "Ready (Count)" : "Ready (Weigh)");
}

void ScaleLogic_SaveConfig(const ScaleState_t *state) {
    if (state->average_item_weight_g > 0.001f) {
        if (hal_Storage_Save_Float(NVS_NAMESPACE, NVS_KEY_SAMPLE_WT, state->average_item_weight_g)) {
            ESP_LOGI(TAG, "Saved average item weight: %.3f g", state->average_item_weight_g);
        } else {
            ESP_LOGE(TAG, "Failed to save average item weight to NVS!");
        }
    } else {
         // Optionally erase the key if the weight is zero/invalid
         // hal_Storage_Erase_Key(NVS_NAMESPACE, NVS_KEY_SAMPLE_WT);
         ESP_LOGI(TAG, "Average item weight is zero, not saving.");
    }
}


void ScaleLogic_Update(ScaleState_t *state, const LoadCellReading_t* reading) {
    // Update basic state from reading
    state->current_weight_g = reading->weight_grams;
    state->is_stable = reading->is_stable; // Assume HAL provides stability state now
    state->is_overload = reading->is_overload;

    // Handle Overload Condition
    if (state->is_overload) {
        state->current_mode = MODE_ERROR;
        state->item_count = 0;
        set_status(state, "OVERLOAD!");
        return; // Skip further processing in overload state
    } else if (state->current_mode == MODE_ERROR && !state->is_overload) {
        // Recover from overload if weight is back in range
        state->current_mode = (state->average_item_weight_g > 0.001f) ? MODE_COUNTING : MODE_WEIGHING;
    }

    // Update Item Count (only in Counting mode and if stable)
    if (state->current_mode == MODE_COUNTING && state->is_stable) {
        if (state->average_item_weight_g > 0.001f) {
            // Ensure weight is positive and significant enough
            if (state->current_weight_g >= state->average_item_weight_g / 2.0f) {
                 // Calculate count using rounding
                state->item_count = (int32_t)roundf(state->current_weight_g / state->average_item_weight_g);
            } else {
                state->item_count = 0; // Treat small weights as zero items
            }
        } else {
            // Average weight not set or invalid
            state->item_count = 0;
            set_status(state, "Set Sample Wt");
        }
    } else if (state->current_mode == MODE_WEIGHING) {
        state->item_count = 0; // No counting in weighing mode
    }
    // If not stable, the count typically holds its last value until stability is achieved again.

    // Update Status Message based on current state (if not already set by specific actions)
    if (state->current_mode != MODE_ERROR && state->current_mode != MODE_SET_SAMPLE) {
         if (!state->is_stable) {
             set_status(state, "..."); // Indicate instability
         } else if (state->current_mode == MODE_COUNTING && state->average_item_weight_g < 0.001f) {
             set_status(state, "Set Sample Wt");
         }
          else if (state->current_mode == MODE_COUNTING) {
             set_status(state, "Stable (Count)");
         } else {
             set_status(state, "Stable (Weigh)");
         }
    }
}

void ScaleLogic_RequestTare(ScaleState_t *state) {
    if (state->current_mode == MODE_ERROR) return; // Don't tare if overloaded

    ESP_LOGI(TAG, "Tare requested.");
    hal_LoadCell_Tare();
    // State update (weight, count) will happen in the next ScaleLogic_Update call
    set_status(state, "Taring...");
    // Optionally: Force immediate read and update after tare? Depends on HAL speed.
}

void ScaleLogic_RequestSetSample(ScaleState_t *state) {
     if (state->current_mode == MODE_ERROR) return; // Don't set sample if overloaded

    if (state->is_stable && state->current_weight_g >= MIN_SAMPLE_WEIGHT_G) {
        state->average_item_weight_g = state->current_weight_g;
        state->current_mode = MODE_COUNTING; // Switch to counting mode
        ESP_LOGI(TAG, "Sample weight set: %.3f g", state->average_item_weight_g);
        set_status(state, "Sample Set");
        ScaleLogic_SaveConfig(state); // Save the new average weight
        // Recalculate count immediately
        ScaleLogic_Update(state, &(LoadCellReading_t){
            .weight_grams = state->current_weight_g, // Use current stable weight
            .is_stable = true,
            .is_overload = false,
            .raw_value = 0 // Raw value not strictly needed here
        });
    } else if (!state->is_stable) {
        ESP_LOGW(TAG, "Cannot set sample: Scale not stable.");
        set_status(state, "Unstable!");
    } else {
         ESP_LOGW(TAG, "Cannot set sample: Weight %.3f g is below minimum %.3f g", state->current_weight_g, MIN_SAMPLE_WEIGHT_G);
         set_status(state, "Wt Too Low");
    }
}

void ScaleLogic_RequestToggleMode(ScaleState_t *state) {
    if (state->current_mode == MODE_ERROR) return; // Cannot change mode if overloaded

    if (state->current_mode == MODE_COUNTING) {
        state->current_mode = MODE_WEIGHING;
        state->item_count = 0; // Reset count when switching to weighing
        ESP_LOGI(TAG, "Switched to Weighing Mode.");
        set_status(state, "Weigh Mode");
    } else if (state->current_mode == MODE_WEIGHING) {
        if (state->average_item_weight_g > 0.001f) {
            state->current_mode = MODE_COUNTING;
            ESP_LOGI(TAG, "Switched to Counting Mode.");
            set_status(state, "Count Mode");
            // Recalculate count immediately based on current weight
             ScaleLogic_Update(state, &(LoadCellReading_t){
                .weight_grams = state->current_weight_g, // Use current weight
                .is_stable = state->is_stable,           // Use current stability
                .is_overload = false,
                .raw_value = 0
             });
        } else {
            ESP_LOGW(TAG, "Cannot switch to Counting Mode: Sample weight not set.");
            set_status(state, "Set Sample Wt");
        }
    }
}
