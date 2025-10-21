#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* ---------- LED pin: adjust for your board ---------- */
#ifndef LED_PIN
#define LED_PIN 2  /* GPIO2 is common on many ESP8266 boards */
#endif
/* ---------------------------------------------------- */

static const char *TAG = "lab2";

/* Per-task priorities (edit for Q3 experiments) */
static const int PRIO_TASK1_LED_ON   = 3;  /* highest */
static const int PRIO_TASK2_LED_OFF  = 2;
static const int PRIO_TASK3_STATUS   = 1;

static SemaphoreHandle_t g_ledMutex;

/* --- GPIO helpers (ESP8266 RTOS SDK style) --- */
static void led_init(void) {
    gpio_config_t io = {0};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << LED_PIN;
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0);
}
static inline void led_on(void)  { gpio_set_level(LED_PIN, 1); }
static inline void led_off(void) { gpio_set_level(LED_PIN, 0); }

/* Task 1: Turn LED ON, actively wait 0.5 s, then yield */
static void task_led_on_busywait(void *arg) {
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

        /* Actively wait for 0.5 s (busy loop), then yield once */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < half_sec) {
            /* busy-wait; scheduler tick still advances */
        }

        xSemaphoreGive(g_ledMutex);
        ESP_LOGI(TAG, "T1: released LED mutex, yielding");
        taskYIELD();  /* explicitly yield after the busy period */
    }
}

/* Task 2: Turn LED OFF, then vTaskDelay(1 s) */
static void task_led_off_delay(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);

    for (;;) {
        xSemaphoreTake(g_ledMutex, portMAX_DELAY);
        ESP_LOGI(TAG, "T2: took LED mutex");
        led_off();
        ESP_LOGI(TAG, "T2: LED OFF (delay 1000 ms)");
        xSemaphoreGive(g_ledMutex);
        vTaskDelay(one_sec);
    }
}

/* Task 3: Print status every 1 s over UART */
static void task_status_uart(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);

    for (;;) {
        ESP_LOGI(TAG, "T3: status ticks=%lu",
                 (unsigned long)xTaskGetTickCount());
        vTaskDelay(one_sec);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main: init");
    led_init();

    g_ledMutex = xSemaphoreCreateMutex();
    configASSERT(g_ledMutex != NULL);

    xTaskCreate(task_led_on_busywait, "tLED_ON",  1024, NULL, PRIO_TASK1_LED_ON,  NULL);
    xTaskCreate(task_led_off_delay,   "tLED_OFF", 1024, NULL, PRIO_TASK2_LED_OFF, NULL);
    xTaskCreate(task_status_uart,     "tUART",    1024, NULL, PRIO_TASK3_STATUS,  NULL);

    /* In ESP8266_RTOS_SDK the scheduler is started by the framework */
}
