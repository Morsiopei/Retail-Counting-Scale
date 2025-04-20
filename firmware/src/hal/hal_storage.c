#include "hal_interfaces.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "HAL_STORAGE";
static bool nvs_initialized = false;

void hal_Storage_Init(void) {
    // Initialize NVS (Non-Volatile Storage). This is often done once in app_main.
    // If called multiple times, it should be safe.
    if (nvs_initialized) return;

    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated or needs formatting, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    nvs_initialized = true;
    ESP_LOGI(TAG, "NVS Initialized.");
}

bool hal_Storage_Save_Float(const char* namespace, const char* key, float value) {
    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized.");
        return false;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return false;
    }

    // NVS doesn't store floats directly, store as uint32_t (bitwise representation)
    uint32_t value_int = *((uint32_t*)&value); // Type punning
    err = nvs_set_u32(nvs_handle, key, value_int);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing value for key '%s'", esp_err_to_name(err), key);
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing changes", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle);
    ESP_LOGD(TAG, "Saved float (as u32) for key '%s' = %f", key, value);
    return true;
}

bool hal_Storage_Load_Float(const char* namespace, const char* key, float* value) {
     if (!nvs_initialized || !value) {
        ESP_LOGE(TAG, "NVS not initialized or null value pointer.");
        return false;
    }
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle for reading. Assuming key '%s' not found.", esp_err_to_name(err), key);
        return false; // Treat as key not found if namespace cannot be opened
    }

    uint32_t value_int = 0;
    err = nvs_get_u32(nvs_handle, key, &value_int);
     nvs_close(nvs_handle); // Close handle regardless of success

    if (err == ESP_OK) {
        *value = *((float*)&value_int); // Type punning back to float
         ESP_LOGD(TAG, "Loaded float (from u32) for key '%s' = %f", key, *value);
        return true;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Key '%s' not found in NVS namespace '%s'.", key, namespace);
        return false;
    } else {
        ESP_LOGE(TAG, "Error (%s) reading value for key '%s'", esp_err_to_name(err), key);
        return false;
    }
}

// Implement Save/Load for other types (u8, i32, string, blob) similarly using nvs_set/get functions

bool hal_Storage_Erase_Key(const char* namespace, const char* key){
     if (!nvs_initialized) return false;
     nvs_handle_t nvs_handle;
     esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
     if (err != ESP_OK) return false;
     err = nvs_erase_key(nvs_handle, key);
     if(err == ESP_OK) {
        err = nvs_commit(nvs_handle);
     }
     nvs_close(nvs_handle);
     return (err == ESP_OK);
}

bool hal_Storage_Erase_Namespace(const char* namespace){
    if (!nvs_initialized) return false;
     nvs_handle_t nvs_handle;
     esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
     if (err != ESP_OK) return false;
     err = nvs_erase_all(nvs_handle);
      if(err == ESP_OK) {
        err = nvs_commit(nvs_handle);
     }
     nvs_close(nvs_handle);
     return (err == ESP_OK);
}

// --- System HAL Functions (Often map directly to RTOS/SDK) ---
void hal_System_DelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

uint64_t hal_System_GetTickMs(void) {
    return (uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void hal_System_Reboot(void) {
    ESP_LOGW(TAG,"Rebooting system...");
    esp_restart();
}
