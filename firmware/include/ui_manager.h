#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "scale_logic.h" // Include for ScaleState_t
#include "hal_interfaces.h" // Include for ButtonEvent_t

void UIManager_Init(const ScaleState_t *initial_state);
void UIManager_UpdateDisplay(const ScaleState_t *state);
void UIManager_HandleInput(ScaleState_t *state, ButtonEvent_t event); // Note: Modifies state

#endif // UI_MANAGER_H
