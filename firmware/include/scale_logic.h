#ifndef SCALE_LOGIC_H
#define SCALE_LOGIC_H

#include "hal_interfaces.h" // Include HAL for types like LoadCellReading_t
#include <stdbool.h>
#include <stdint.h>
// If using FreeRTOS:
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // For mutexes

typedef enum {
    MODE_WEIGHING,
    MODE_COUNTING,
    MODE_SET_SAMPLE, // Intermediate state while setting sample
    MODE_ERROR
} ScaleMode_t;

// Structure to hold the overall state of the scale
// This can be passed between tasks or accessed via mutex
typedef struct {
    float current_weight_g;
    int32_t item_count;
    float average_item_weight_g;
    bool is_stable;
    bool is_overload;
    ScaleMode_t current_mode;
    char status_message[32]; // For short status strings on UI

    // RTOS synchronization (if needed)
    // SemaphoreHandle_t mutex; // To protect access to this struct from multiple tasks

    // Internal state for stability check
    float recent_weights[STABLE_READING_COUNT];
    int stable_counter;
    int weight_history_index;

} ScaleState_t;


void ScaleLogic_Init(ScaleState_t *state);
void ScaleLogic_Update(ScaleState_t *state, const LoadCellReading_t* reading);
void ScaleLogic_RequestTare(ScaleState_t *state);
void ScaleLogic_RequestSetSample(ScaleState_t *state);
void ScaleLogic_RequestToggleMode(ScaleState_t *state);
void ScaleLogic_LoadConfig(ScaleState_t *state); // Load avg weight from storage
void ScaleLogic_SaveConfig(const ScaleState_t *state); // Save avg weight to storage

#endif // SCALE_LOGIC_H
