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
        int free_heap = get_free_heap();

        int raw = (int)(temperature * 100);
        int avg = (int)(avg_temp * 100);

        char buf[64];

        print_debug("RESULT,sensor_test,wasm,");
        itoa_simple(BASE_RUN_ID + i, buf); print_debug(buf); print_debug(",");
        itoa_simple(raw, buf);              print_debug(buf); print_debug(",");
        itoa_simple(avg, buf);              print_debug(buf); print_debug(",");
        itoa_simple((int)latency, buf);     print_debug(buf); print_debug(",");
        itoa_simple(free_heap, buf);        print_debug(buf);

        enter_light_sleep();
    }

    return 0;
}
