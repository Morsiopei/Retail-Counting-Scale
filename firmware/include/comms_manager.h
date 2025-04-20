#ifndef COMMS_MANAGER_H
#define COMMS_MANAGER_H

#include "scale_logic.h" // Include for ScaleState_t
#include <stdbool.h>

typedef enum {
    COMMS_STATE_DISCONNECTED,
    COMMS_STATE_CONNECTING,
    COMMS_STATE_CONNECTED,
    COMMS_STATE_SENDING,
    COMMS_STATE_ERROR
} CommsState_t;

void CommsManager_Init(void);
CommsState_t CommsManager_GetCurrentState(void);
void CommsManager_Connect(void); // Non-blocking request to connect
void CommsManager_SendData(const ScaleState_t *state); // Formats and sends data if connected
void CommsManager_RunPeriodic(void); // Handles state machine logic (call this periodically from task)

#endif // COMMS_MANAGER_H
