#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#ifndef LED_PIN
#define LED_PIN 2
#endif

static const char *TAG = "lab2";

static const int PRIO_TASK1_LED_ON   = 3;  /* highest */
static const int PRIO_TASK2_LED_OFF  = 2;
static const int PRIO_TASK3_STATUS   = 1;

/* Use a MUTEX as the LED lock (priority inheritance ENABLED) */
static SemaphoreHandle_t g_ledMutex;

static void led_init(void) {
    gpio_config_t io = (gpio_config_t){0};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << LED_PIN;
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0);
}
static inline void led_on(void)  { gpio_set_level(LED_PIN, 1); }
static inline void led_off(void) { gpio_set_level(LED_PIN, 0); }

/* T1: LED ON, actively wait 0.5 s, then yield + block 1 tick */
static void task_led_on(void *arg) {
    (void)arg;
    const TickType_t half_sec = pdMS_TO_TICKS(500);
    for (;;) {
        TickType_t req = xTaskGetTickCount();
        xSemaphoreTake(g_ledMutex, portMAX_DELAY);
        TickType_t got = xTaskGetTickCount();
        ESP_LOGI(TAG, "T1: took LED mutex (wait=%lu ms)",
                 (unsigned long)((got - req) * portTICK_PERIOD_MS));

        led_on();
        ESP_LOGI(TAG, "T1: LED ON (busy-wait 500 ms)");
        xSemaphoreGive(g_ledMutex);

        /* active wait WITHOUT holding the lock */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < half_sec) { /* spin */ }

        taskYIELD();
        vTaskDelay(1); /* ensure lower-priority tasks get CPU */
    }
}

/* T2: LED OFF, then delay 1 s */
static void task_led_off(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);
    for (;;) {
        TickType_t req = xTaskGetTickCount();
        xSemaphoreTake(g_ledMutex, portMAX_DELAY);
        TickType_t got = xTaskGetTickCount();
        ESP_LOGI(TAG, "T2: took LED mutex (wait=%lu ms)",
                 (unsigned long)((got - req) * portTICK_PERIOD_MS));

        led_off();
        ESP_LOGI(TAG, "T2: LED OFF (delay 1000 ms)");
        xSemaphoreGive(g_ledMutex);

        vTaskDelay(one_sec);
    }
}

/* T3: status every 1 s */
static void task_status(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);
    for (;;) {
        ESP_LOGI(TAG, "T3: tick=%lu", (unsigned long)xTaskGetTickCount());
        vTaskDelay(one_sec);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main: init");
    led_init();

    /* Mutex == priority inheritance enabled */
    g_ledMutex = xSemaphoreCreateMutex();
    configASSERT(g_ledMutex != NULL);

    xTaskCreate(task_led_on,  "tLED_ON",  1024, NULL, PRIO_TASK1_LED_ON,  NULL);
    xTaskCreate(task_led_off, "tLED_OFF", 1024, NULL, PRIO_TASK2_LED_OFF, NULL);
    xTaskCreate(task_status,  "tSTATUS",  1024, NULL, PRIO_TASK3_STATUS,  NULL);
}
