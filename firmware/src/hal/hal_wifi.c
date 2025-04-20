#include "hal_interfaces.h"
#include "scale_config.h"
#include <string.h>
// --- ESP-IDF Includes ---
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

static const char *TAG = "HAL_WIFI";

// --- ESP-IDF WiFi Event Handling ---
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT      = BIT1;
static int s_retry_num = 0;
#define WIFI_MAXIMUM_RETRY  5

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi STA Started, connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry WiFi connection (%d/%d)...", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi connection failed after maximum retries.");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // Reset retry counter on successful connection
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void hal_Wifi_Init(void) {
    ESP_LOGI(TAG, "Initializing WiFi HAL...");
    // Initialize NVS (needed for WiFi config storage) - often done once in main
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init()); // Initialize TCP/IP stack

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Create default event loop
    esp_netif_create_default_wifi_sta(); // Create default Station interface

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Initialize WiFi driver

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi HAL Initialized.");
}

bool hal_Wifi_Connect(const char* ssid, const char* password, uint32_t timeout_ms) {
     ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
    s_retry_num = 0; // Reset retry counter for this attempt
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT); // Clear previous results


    wifi_config_t wifi_config = {0}; // Important to zero initialize
     strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
     strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
     wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // Adjust as needed
     wifi_config.sta.pmf_cfg.capable = true;
     wifi_config.sta.pmf_cfg.required = false;


    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // esp_wifi_connect() is called by the event handler on WIFI_EVENT_STA_START

    // Wait for connection or failure event
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, // Clear bits on exit = false
            pdFALSE, // Wait for ANY bit = false (wait for either connect OR fail)
            pdMS_TO_TICKS(timeout_ms));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", ssid);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", ssid);
        return false;
    } else {
        ESP_LOGE(TAG, "Connection timed out to SSID:%s", ssid);
        // Should we stop the connection attempt explicitly?
        // esp_wifi_disconnect(); // Maybe? Or let retry handler manage?
        return false;
    }
}

bool hal_Wifi_IsConnected(void) {
    if (!wifi_event_group) return false;
    // Check the event group bit OR query the interface status
     wifi_ap_record_t ap_info;
     esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
     return (err == ESP_OK); // Returns ESP_OK if associated
    // return (xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT) != 0; // Alternative check
}

void hal_Wifi_Disconnect(void) {
    ESP_LOGI(TAG,"Disconnecting WiFi...");
    esp_wifi_disconnect();
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT); // Manually clear bit on disconnect request
}


// --- HTTP Client Event Handler ---
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Collect data into response buffer passed to hal_Wifi_HttpPost
             if (evt->user_data) { // Check if user_data (response buffer struct) is provided
                 char* resp_buf = ((HttpUserData*)evt->user_data)->buffer;
                 size_t resp_buf_size = ((HttpUserData*)evt->user_data)->buffer_size;
                 size_t* resp_len = &(((HttpUserData*)evt->user_data)->current_len);

                 if (*resp_len + evt->data_len < resp_buf_size) {
                     memcpy(resp_buf + *resp_len, evt->data, evt->data_len);
                     *resp_len += evt->data_len;
                     resp_buf[*resp_len] = '\0'; // Null-terminate
                 } else {
                      ESP_LOGW(TAG,"HTTP Response buffer overflow!");
                 }
             }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            output_buffer = NULL;
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            // Clean up?
            break;
        // case HTTP_EVENT_REDIRECT: // Added in later ESP-IDF versions
        //     ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        //     break;
    }
    return ESP_OK;
}

// Helper struct to pass response buffer details to HTTP event handler
typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t current_len;
} HttpUserData;


int hal_Wifi_HttpPost(const char* url, const char* payload, char* response_buffer, size_t buffer_size, uint32_t timeout_ms) {
     if (!hal_Wifi_IsConnected()) {
         ESP_LOGE(TAG,"HTTP Post failed: WiFi not connected.");
         return -1; // Use negative numbers for connection errors
     }

     if (!url || !payload || !response_buffer || buffer_size == 0) {
          ESP_LOGE(TAG,"HTTP Post failed: Invalid arguments.");
          return -2;
     }

    esp_http_client_handle_t client = NULL;
    esp_err_t err;
    int http_status = -1; // Default to error

    // Prepare user data for event handler
    HttpUserData user_data = {
        .buffer = response_buffer,
        .buffer_size = buffer_size,
        .current_len = 0
    };
    response_buffer[0] = '\0'; // Ensure buffer is initially empty

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = (int)timeout_ms,
        .event_handler = _http_event_handler,
        .user_data = &user_data, // Pass buffer info to handler
        // .crt_bundle_attach = esp_crt_bundle_attach, // For HTTPS
    };

    client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return -3;
    }

    // Set headers and payload
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    ESP_LOGD(TAG, "Performing HTTP POST to %s", url);
    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        http_status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                http_status,
                esp_http_client_get_content_length(client));
        // Response data should have been collected in response_buffer by the event handler
        ESP_LOGD(TAG, "Response Body: %s", response_buffer);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        http_status = -4; // Indicate perform error
    }

    esp_http_client_cleanup(client);
    return http_status;
}
