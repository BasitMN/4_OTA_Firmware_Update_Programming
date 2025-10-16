/*
 * DHT11 Sensor Driver for ESP32
 * Based on: https://github.com/UncleRus/esp-idf-lib
 */

#ifndef DHT11_H_
#define DHT11_H_

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHT11 sensor reading result
 */
typedef struct {
    float temperature;  ///< Temperature in Celsius
    float humidity;     ///< Relative humidity in percent
} dht11_reading_t;

/**
 * @brief Read data from DHT11 sensor
 * 
 * @param gpio_num GPIO pin number where DHT11 data line is connected
 * @param reading Pointer to structure to store reading results
 * @return ESP_OK on success
 */
esp_err_t dht11_read(gpio_num_t gpio_num, dht11_reading_t *reading);

#ifdef __cplusplus
}
#endif

#endif // DHT11_H_
