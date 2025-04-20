#include "hal_interfaces.h"
#include "scale_config.h"
#include "driver/gpio.h" // ESP-IDF GPIO driver
#include "freertos/FreeRTOS.h" // For ticks
#include "freertos/timers.h"   // For debounce if using timers
#include "esp_log.h"

static const char *TAG = "HAL_BUTTONS";

// Simple debounce state
typedef struct {
    gpio_num_t pin;
    uint32_t last_press_time;
    bool last_state; // true = pressed (LOW for pull-up)
} ButtonInfo_t;

static ButtonInfo_t tare_button = { .pin = BUTTON_TARE_PIN, .last_press_time = 0, .last_state = false };
static ButtonInfo_t sample_button = { .pin = BUTTON_SAMPLE_PIN, .last_press_time = 0, .last_state = false };
static ButtonInfo_t mode_button = { .pin = BUTTON_MODE_PIN, .last_press_time = 0, .last_state = false };

#define DEBOUNCE_DELAY_MS 50

void hal_Buttons_Init(void) {
    ESP_LOGI(TAG, "Initializing Button GPIOs...");
    // TODO: Configure GPIO pins for buttons as inputs with pull-ups
    // Example ESP-IDF:
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUTTON_TARE_PIN) | (1ULL << BUTTON_SAMPLE_PIN) | (1ULL << BUTTON_MODE_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Assuming buttons connect pin to GND when pressed
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Using polling here
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Button GPIOs Initialized.");
}

// Simple polling debounce check
static bool check_button(ButtonInfo_t *button) {
    bool pressed_event = false;
    // Buttons assumed active LOW (pressed = 0) due to internal pull-up
    bool current_physical_state = (gpio_get_level(button->pin) == 0);
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (current_physical_state != button->last_state) {
        // State changed, potential press/release
        if (now - button->last_press_time > DEBOUNCE_DELAY_MS) {
            if (current_physical_state) { // Went from released to pressed
                pressed_event = true; // Register the press event
                ESP_LOGD(TAG, "Button %d Pressed Event", button->pin);
            } else {
                 // Went from pressed to released - do nothing on release for this simple logic
                 ESP_LOGD(TAG, "Button %d Released", button->pin);
            }
            button->last_state = current_physical_state;
            button->last_press_time = now; // Update time only after debounce period confirms change
        }
        // Else: state changed within debounce period, ignore as noise
    }
    // else: state hasn't changed, do nothing

    return pressed_event; // Return true only on a debounced press event
}


ButtonEvent_t hal_Buttons_Read(void) {
    if (check_button(&tare_button)) {
        return BUTTON_TARE_PRESS;
    }
    if (check_button(&sample_button)) {
        return BUTTON_SAMPLE_PRESS;
    }
    if (check_button(&mode_button)) {
        return BUTTON_MODE_PRESS;
    }

    // TODO: Add logic for HOLD events if needed
    // (Check if button->last_state is still true and now - button->last_press_time > HOLD_DURATION_MS)

    return BUTTON_NONE;
}
