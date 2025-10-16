/*
 * Simple DHT11 Sensor Example
 * Reads temperature and humidity from DHT11 sensor connected to GPIO 4
 * Board: ESP32-S3-DevKitC-1
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "dht11.h"

#define DHT11_GPIO GPIO_NUM_4  // GPIO 4 is safe on ESP32-S3
#define READ_INTERVAL_MS 3000  // Read every 3 seconds (DHT11 minimum is 2 seconds)

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "DHT11 Sensor Example");
    ESP_LOGI(TAG, "Board: ESP32-S3-DevKitC-1");
    ESP_LOGI(TAG, "DHT11 connected to GPIO %d", DHT11_GPIO);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Wiring for ESP32-S3-DevKitC-1:");
    ESP_LOGI(TAG, "  DHT11 VCC  -> ESP32-S3 3V3 (Pin 2) or 5V (Pin 1)");
    ESP_LOGI(TAG, "  DHT11 DATA -> ESP32-S3 GPIO %d (Pin 5)", DHT11_GPIO);
    ESP_LOGI(TAG, "  DHT11 GND  -> ESP32-S3 GND (Pin 3 or 38)");
    ESP_LOGI(TAG, "  Pull-up: 4.7k-10k resistor between DATA and VCC");
    ESP_LOGI(TAG, "========================================");

    // Reset GPIO to known state
    gpio_reset_pin(DHT11_GPIO);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Test GPIO first
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);
    
    ESP_LOGI(TAG, "Testing GPIO %d...", DHT11_GPIO);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Read GPIO level multiple times
    for (int i = 0; i < 3; i++) {
        int level = gpio_get_level(DHT11_GPIO);
        ESP_LOGI(TAG, "GPIO %d read #%d: %d (should be 1 with pull-up)", DHT11_GPIO, i+1, level);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    int level = gpio_get_level(DHT11_GPIO);
    if (level == 0) {
        ESP_LOGE(TAG, "❌ CRITICAL: GPIO is stuck LOW!");
        ESP_LOGE(TAG, "Possible causes:");
        ESP_LOGE(TAG, "  1. DHT11 DATA pin not connected to GPIO %d", DHT11_GPIO);
        ESP_LOGE(TAG, "  2. DHT11 is not powered (check VCC and GND)");
        ESP_LOGE(TAG, "  3. No pull-up resistor (4.7k-10k between DATA and VCC)");
        ESP_LOGE(TAG, "  4. Faulty DHT11 sensor");
        ESP_LOGE(TAG, "  5. Short circuit on DATA line");
    } else {
        ESP_LOGI(TAG, "✓ GPIO level is HIGH - wiring looks OK");
    }

    dht11_reading_t reading;

    // Wait for sensor to stabilize
    ESP_LOGI(TAG, "Waiting 2 seconds for sensor to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    int success_count = 0;
    int fail_count = 0;

    while (1) {
        ESP_LOGI(TAG, "=== Reading DHT11 (Success: %d, Fail: %d) ===", success_count, fail_count);
        
        esp_err_t result = dht11_read(DHT11_GPIO, &reading);
        
        if (result == ESP_OK) {
            success_count++;
            printf("✓ Temperature: %.1f°C\n", reading.temperature);
            printf("✓ Humidity: %.1f%%\n", reading.humidity);
            printf("\n");
        } else {
            fail_count++;
            const char* error_msg;
            switch(result) {
                case ESP_ERR_TIMEOUT:
                    error_msg = "Timeout - sensor not responding (check wiring/power)";
                    break;
                case ESP_ERR_INVALID_CRC:
                    error_msg = "Checksum error - data corrupted (check connections)";
                    break;
                default:
                    error_msg = "Unknown error";
                    break;
            }
            ESP_LOGE(TAG, "✗ Failed: %s (error: %d)", error_msg, result);
            
            // Re-check GPIO after failure
            gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
            vTaskDelay(pdMS_TO_TICKS(10));
            int gpio_level = gpio_get_level(DHT11_GPIO);
            ESP_LOGI(TAG, "GPIO %d level after failure: %d", DHT11_GPIO, gpio_level);
        }

        // Wait before next reading (minimum 2 seconds for DHT11)
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

