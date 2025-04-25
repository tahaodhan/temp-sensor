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
void print_debug(const char *msg);

#define FILTER_SIZE 5
#define RUN_COUNT 10
#define BASE_RUN_ID 101

void itoa_simple(int value, char *buffer) {
    int i = 0;
    char temp[12];
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }
    for (int j = 0; j < i; j++) {
        buffer[j] = temp[i - j - 1];
    }
    buffer[i] = '\0';
}

int main(int argc, char **argv) {
    temp_sensor_init();

    float readings[FILTER_SIZE] = {0};
    int index = 0;
    float sum = 0;

    for (int i = 0; i < RUN_COUNT; i++) {
        float temperature = temp_sensor_read();
        uint64_t start_time = get_timestamp();

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
    }

    return 0;
}
