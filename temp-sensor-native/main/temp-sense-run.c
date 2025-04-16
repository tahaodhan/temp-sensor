#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/temp_sensor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"
#include "esp_pm.h"

#define RUN_COUNT 10
#define BASE_RUN_ID 1
#define FILTER_SIZE 5

static const char *TAG = "TEMP_SENSOR";

uint64_t get_timestamp() {
    return esp_timer_get_time();
}

void app_main(void) {
    temp_sensor_config_t temp_sensor = {
        .dac_offset = TSENS_DAC_L2,
        .clk_div = 6,
    };
    ESP_ERROR_CHECK(temp_sensor_set_config(temp_sensor));
    ESP_ERROR_CHECK(temp_sensor_start());

    float readings[FILTER_SIZE] = {0};
    int index = 0;
    float sum = 0;

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
for (int i = 0; i < RUN_COUNT; i++) {
        float temperature;
        uint64_t start_time = get_timestamp();

        ESP_ERROR_CHECK(temp_sensor_read_celsius(&temperature));

        sum -= readings[index];
        readings[index] = temperature;
        sum += temperature;
        index = (index + 1) % FILTER_SIZE;

        float avg_temp = sum / FILTER_SIZE;

        uint64_t end_time = get_timestamp();
        uint64_t latency = end_time - start_time;


        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

        // ESP_LOGI(TAG, "Raw Temp: %.2f C, Filtered Temp: %.2f C, Latency: %llu us", temperature, avg_temp, latency);
        ESP_LOGI(TAG, "RESULT,sensor_test,native,%d,%.2f,%.2f,%llu,%u",
             BASE_RUN_ID + i, temperature, avg_temp, latency, free_heap);


        esp_sleep_enable_timer_wakeup(1000000);  


        //Sleep
        ESP_LOGI(TAG, "Entering light sleep...");
        esp_light_sleep_start();  
        ESP_LOGI(TAG, "Woke up from light sleep.");

        vTaskDelay(pdMS_TO_TICKS(100));
    }

}