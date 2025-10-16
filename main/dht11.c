/*
 * DHT11 Sensor Driver for ESP32
 * Based on: https://github.com/UncleRus/esp-idf-lib
 * 
 * DHT11 Protocol:
 * 1. MCU sends start signal (LOW for 18ms, then HIGH for 20-40us)
 * 2. DHT11 responds with LOW for 80us, then HIGH for 80us
 * 3. DHT11 sends 40 bits of data (5 bytes):
 *    - Byte 0: Humidity integer part
 *    - Byte 1: Humidity decimal part (always 0 for DHT11)
 *    - Byte 2: Temperature integer part
 *    - Byte 3: Temperature decimal part (always 0 for DHT11)
 *    - Byte 4: Checksum (sum of bytes 0-3)
 * 4. Each bit starts with 50us LOW, then:
 *    - 26-28us HIGH = '0'
 *    - 70us HIGH = '1'
 */

#include "dht11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT11";

// Global variables to store latest sensor readings (shared with other tasks)
static float current_humidity = 0.0f;
static float current_temperature = 0.0f;

/**
 * @brief Wait for GPIO pin to reach specified level with timeout
 */
static esp_err_t wait_for_level(gpio_num_t gpio_num, int level, int timeout_us)
{
    int elapsed = 0;
    while (gpio_get_level(gpio_num) != level) {
        if (elapsed++ > timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
        ets_delay_us(1);
    }
    return ESP_OK;
}

/**
 * @brief Measure pulse duration in microseconds
 */
static int measure_pulse(gpio_num_t gpio_num, int level, int timeout_us)
{
    int elapsed = 0;
    while (gpio_get_level(gpio_num) == level) {
        if (elapsed++ > timeout_us) {
            return -1;
        }
        ets_delay_us(1);
    }
    return elapsed;
}

esp_err_t dht11_read(gpio_num_t gpio_num, dht11_reading_t *reading)
{
    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[5] = {0};

    // Configure GPIO as output
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(gpio_num, 1);
    ets_delay_us(1000); // Brief stabilization

    // Send start signal
    gpio_set_level(gpio_num, 0);
    ets_delay_us(20000); // 20ms LOW
    gpio_set_level(gpio_num, 1);
    ets_delay_us(40); // 40us HIGH

    // Switch to input mode
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);

    // Wait for DHT11 response (80us LOW + 80us HIGH)
    if (wait_for_level(gpio_num, 0, 100) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for response LOW");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_level(gpio_num, 1, 100) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for response HIGH");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_level(gpio_num, 0, 100) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout waiting for data start");
        return ESP_ERR_TIMEOUT;
    }

    // Read 40 bits (5 bytes)
    for (int i = 0; i < 40; i++) {
        // Wait for bit start (50us LOW)
        if (wait_for_level(gpio_num, 1, 100) != ESP_OK) {
            ESP_LOGE(TAG, "Timeout waiting for bit %d start", i);
            return ESP_ERR_TIMEOUT;
        }

        // Measure HIGH pulse duration
        int duration = measure_pulse(gpio_num, 1, 100);
        if (duration < 0) {
            ESP_LOGE(TAG, "Timeout measuring bit %d pulse", i);
            return ESP_ERR_TIMEOUT;
        }

        // Determine bit value (>40us = 1, <40us = 0)
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if (duration > 40) {
            data[byte_idx] |= (1 << bit_idx);
        }
    }

    // Verify checksum
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "Checksum error: calculated 0x%02X, received 0x%02X", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }

    // Parse data (DHT11 only uses integer parts)
    reading->humidity = (float)data[0];
    reading->temperature = (float)data[2];

    // Update global variables for access by other tasks (HTTP server, etc.)
    current_humidity = reading->humidity;
    current_temperature = reading->temperature;

    ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", 
             reading->temperature, reading->humidity);

    return ESP_OK;
}

// == Getter functions for accessing sensor data from other tasks ==

float dht11_get_humidity(void)
{
    return current_humidity;
}

float dht11_get_temperature(void)
{
    return current_temperature;
}
