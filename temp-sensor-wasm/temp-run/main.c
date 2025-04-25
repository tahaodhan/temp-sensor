#include <stdio.h>
#include <stdint.h>

__attribute__((import_module("env"), import_name("temp_sensor_init")))
void temp_sensor_init(void);

__attribute__((import_module("env"), import_name("temp_sensor_read")))
float temp_sensor_read(void);

__attribute__((import_module("env"), import_name("get_timestamp")))
uint64_t get_timestamp(void);

__attribute__((import_module("env"), import_name("get_free_heap")))
int get_free_heap(void);

__attribute__((import_module("env"), import_name("enter_light_sleep")))
void enter_light_sleep(void);

__attribute__((import_module("env"), import_name("print_debug")))
void print_debug(const char *message);

#define FILTER_SIZE 5
static const char *TAG = "TEMP_SENSOR";

int main(int argc, char **argv) {
    temp_sensor_init();

    float readings[FILTER_SIZE] = {0};
    int index = 0;
    float sum = 0;

    while (1) {
        float temperature;
        uint64_t start_time = get_timestamp();

        temperature = temp_sensor_read();

        sum -= readings[index];
        readings[index] = temperature;
        sum += temperature;
        index = (index + 1) % FILTER_SIZE;

        float avg_temp = sum / FILTER_SIZE;

        uint64_t end_time = get_timestamp();
        uint64_t latency = end_time - start_time;

        char log_buffer[128];
        // sprintf(log_buffer, "%s: Raw Temp: %.2f C, Filtered Temp: %.2f C, Latency: %llu us", TAG, temperature, avg_temp, latency);
        print_debug(log_buffer);

        if (index % 10 == 0) {
            // sprintf(log_buffer, "%s: Free heap: %d bytes", TAG, get_free_heap());
            print_debug(log_buffer);
        }

        print_debug("TEMP_SENSOR: Entering light sleep...");
        enter_light_sleep();
        print_debug("TEMP_SENSOR: Woke up from light sleep.");
    }

    return 0;
}
