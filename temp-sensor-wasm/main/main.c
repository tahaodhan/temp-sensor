#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "test_wasm.h"
#include "esp_log.h"
#include "driver/temp_sensor.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"

#define LOG_TAG "wamr_temp"

static void wasm_temp_sensor_init(wasm_exec_env_t exec_env) {
    temp_sensor_config_t temp_sensor = {
        .dac_offset = TSENS_DAC_L2,
        .clk_div = 6,
    };
    ESP_ERROR_CHECK(temp_sensor_set_config(temp_sensor));
    ESP_ERROR_CHECK(temp_sensor_start());
}

static float wasm_temp_sensor_read(wasm_exec_env_t exec_env) {
    float temperature;
    ESP_ERROR_CHECK(temp_sensor_read_celsius(&temperature));
    return temperature;
}

static uint64_t wasm_get_timestamp(wasm_exec_env_t exec_env) {
    return esp_timer_get_time();
}

static int wasm_get_free_heap(wasm_exec_env_t exec_env) {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

static void wasm_enter_light_sleep(wasm_exec_env_t exec_env) {
    esp_sleep_enable_timer_wakeup(1000000);
    esp_light_sleep_start();
}

static void wasm_print_debug(wasm_exec_env_t exec_env, const char *message) {
    ESP_LOGI(LOG_TAG, "%s", message);
}

static void *app_instance_main(wasm_module_inst_t module_inst) {
    const char *exception;
    wasm_application_execute_main(module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(module_inst))) {
        ESP_LOGE(LOG_TAG, "WASM Exception: %s", exception);
    }
    return NULL;
}

void *iwasm_main(void *arg) {
    (void)arg;
    uint8_t *wasm_file_buf = (uint8_t *)wasm_test_file_interp;
    unsigned wasm_file_buf_size = sizeof(wasm_test_file_interp);
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];
    void *ret;
    RuntimeInitArgs init_args;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = (void *)os_malloc;
    init_args.mem_alloc_option.allocator.realloc_func = (void *)os_realloc;
    init_args.mem_alloc_option.allocator.free_func = (void *)os_free;

    ESP_LOGI(LOG_TAG, "Initialize WASM runtime");
    if (!wasm_runtime_full_init(&init_args)) {
        ESP_LOGE(LOG_TAG, "Init runtime failed.");
        return NULL;
    }

    ESP_LOGI(LOG_TAG, "Registering native functions");
    NativeSymbol native_symbols[] = {
        {"temp_sensor_init", (void*)wasm_temp_sensor_init, "()", NULL},
        {"temp_sensor_read", (void*)wasm_temp_sensor_read, "()f", NULL},
        {"get_timestamp", (void*)wasm_get_timestamp, "()I", NULL},
        {"get_free_heap", (void*)wasm_get_free_heap, "()i", NULL},
        {"enter_light_sleep", (void*)wasm_enter_light_sleep, "()", NULL},
        {"print_debug", (void*)wasm_print_debug, "($)", NULL}
    };
    wasm_runtime_register_natives("env", native_symbols, sizeof(native_symbols) / sizeof(NativeSymbol));

    ESP_LOGI(LOG_TAG, "Load WASM module");
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_buf_size, error_buf, sizeof(error_buf)))) {
        ESP_LOGE(LOG_TAG, "Error in wasm_runtime_load: %s", error_buf);
        goto fail1;
    }

    ESP_LOGI(LOG_TAG, "Instantiate WASM module");
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, 32 * 1024, 32 * 1024, error_buf, sizeof(error_buf)))) {
        ESP_LOGE(LOG_TAG, "Error while instantiating: %s", error_buf);
        goto fail2;
    }

    ESP_LOGI(LOG_TAG, "Run WASM application");
    ret = app_instance_main(wasm_module_inst);
    assert(!ret);

    wasm_runtime_deinstantiate(wasm_module_inst);
fail2:
    wasm_runtime_unload(wasm_module);
fail1:
    wasm_runtime_destroy();

    return NULL;
}

void app_main(void) {
    pthread_t t;
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&tattr, 4096);

    int res = pthread_create(&t, &tattr, iwasm_main, NULL);
    assert(res == 0);

    res = pthread_join(t, NULL);
    assert(res == 0);

    ESP_LOGI(LOG_TAG, "WASM Temperature Sensor Project Finished.");
}
