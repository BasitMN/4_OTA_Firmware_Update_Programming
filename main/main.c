/**
 * Application entry point.
 */

#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dht11.h"
#include "wifi_app.h"

#define DHT11_GPIO GPIO_NUM_4
#define DHT11_READ_INTERVAL_MS 3000

static const char *TAG = "MAIN";

/**
 * DHT11 sensor reading task
 */
static void dht11_task(void *pvParameter)
{
	ESP_LOGI(TAG, "DHT11 task started on GPIO %d", DHT11_GPIO);
	
	dht11_reading_t reading;
	
	// Wait for sensor to stabilize after power-on
	vTaskDelay(pdMS_TO_TICKS(2000));
	
	while (1)
	{
		esp_err_t result = dht11_read(DHT11_GPIO, &reading);
		
		if (result == ESP_OK)
		{
			ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", 
					 reading.temperature, reading.humidity);
		}
		else
		{
			ESP_LOGE(TAG, "Failed to read DHT11 sensor (error: %d)", result);
		}
		
		// Wait before next reading (DHT11 needs minimum 2 seconds between readings)
		vTaskDelay(pdMS_TO_TICKS(DHT11_READ_INTERVAL_MS));
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Starting application...");
	
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "NVS initialized");

	// Start Wifi
	wifi_app_start();
	ESP_LOGI(TAG, "WiFi started");

	// Start DHT11 Sensor task
	xTaskCreate(&dht11_task, "dht11_task", 4096, NULL, 5, NULL);
	ESP_LOGI(TAG, "DHT11 task created");
}

